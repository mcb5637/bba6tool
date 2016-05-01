// bbaTool.cpp : Defines the entry point for the console application.
//
#pragma pack(4)
#include "stdafx.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <direct.h>
#include <windows.h>

#include "zlib/zlib.h"
#include "s6Crypt.h"
#include "compress.h"
#include "tinydir.h"
#include "s6data.h"
#include "treeReader.h"

#include <eh.h>
#include "CrashHandler.h"
extern "C" {

	static HANDLE conHandle = NULL;
	static char copyBuf[BUFSZ];

//Program Helper
#if 1
	void fatal(const char *fmt, ...)
	{
		va_list argptr;
		va_start(argptr, fmt);
		vprintf(fmt, argptr);
		va_end(argptr);

		getchar();
		getchar();
		exit(EXIT_FAILURE);
	}

	static void mkpath(char *path)
	{
		for (char *p = path; *p != 0; p++) //subdirs
		{
			if (*p == '\\')
			{
				*p = 0;
				_mkdir(path);
				*p = '\\';
			}
		}

		_mkdir(path);
	}

	void gotoxy(COORD p)
	{
		SetConsoleCursorPosition(conHandle, p);
	}

	COORD getCurPos()
	{
		_CONSOLE_SCREEN_BUFFER_INFO  curInfo;
		GetConsoleScreenBufferInfo(conHandle, &curInfo);
		return curInfo.dwCursorPosition;
	}

	void ShowConsoleCursor(BOOL bShow)
	{

		CONSOLE_CURSOR_INFO     cursorInfo;

		cursorInfo.dwSize = 10;
		cursorInfo.bVisible = bShow;

		SetConsoleCursorInfo(conHandle, &cursorInfo);

	}
#endif 

//S6 Helper
#if 1
	uint32_t htSize(uint32_t entries)
	{
		uint32_t counter = entries + 1;
		uint32_t pot = 0;
		do
		{
			counter >>= 1;
			pot++;
		} while (counter);

		uint32_t size = (1 << pot);
		if ((double)entries * 1.5 > (double)size)
			return size * 4;
		else
			return size * 2;
	}

	void *DecompressData(CompressedData *data)
	{
		void *dataBuffer = malloc(data->decompressedSize);
		uncompress((Bytef *)dataBuffer, (uLongf *) &data->decompressedSize, (Bytef *)data->data, data->compressedSize);
		return dataBuffer;
	}

	void S6_InitDir(S6Directory *dirObj)
	{
		dirObj->headerSize = 64;
		dirObj->fileEntriesOffset = (uint32_t)dirObj->entries - (uint32_t)dirObj - 4;
		strncpy(dirObj->archiveHeader, "BAF", 3);
		dirObj->archiveVersion = 4;
		dirObj->unknown5 = 5;
		dirObj->headerSize2 = 64;
		dirObj->headerEncrID = S6_HEAD_CRYPTID;
		dirObj->encryptionTable[1] = S6_FILE_CRYPTID;
		dirObj->unknown1 = 0xD1C81BB5;
	}

	void S6_InitBBAHeader(BBAHeader *bbaHeader)
	{
		strncpy(bbaHeader->archiveHeader, "BAF", 3);
		bbaHeader->archiveVersion = 4;
		bbaHeader->unknown5 = 5;
		bbaHeader->headerSize = 64;
		bbaHeader->headerEncrID = S6_HEAD_CRYPTID;
	}
	
	void S6_UnpackFile(S6DirEntry * entry, FILE *readHandle, FILE *writeHandle)
	{
		//CheckFileExt(entry);

		if (entry->filetype == 1)
		{
			S6FileHeader fileHeader;

			fseek(readHandle, entry->offset, SEEK_SET);
			fread(&fileHeader, sizeof(S6FileHeader), 1, readHandle);
			S6FileHeader_Decrypt(&fileHeader, entry);
			fseek(readHandle, entry->offset+8, SEEK_SET);
			inf(readHandle, writeHandle, &fileHeader);
		}
		else
		{
			fseek(readHandle, entry->offset, SEEK_SET);

			int toCopy = entry->decompressedSize;
			while (toCopy)
			{
				int copySize = toCopy > BUFSZ ? BUFSZ : toCopy;
				fread(copyBuf, 1, copySize, readHandle);
				fwrite(copyBuf, 1, copySize, writeHandle);
				toCopy -= copySize;
			}
		}
	}

	void S6_PrepareFolder(S6DirEntry * entry)
	{
		if (entry->dirPart != 0)
		{
			entry->filename[entry->dirPart - 1] = 0;
			mkpath(entry->filename);
			entry->filename[entry->dirPart - 1] = '\\';
		}
	}

	typedef struct 
	{
		uint32_t filetype;
		char extension[10];
	} FT;

	int ftFilled = 0;
	FT filetypes[50] = { 0 };

	void CheckFileExt(S6DirEntry *entry)
	{
		int estart = entry->filenameLength-1;
		for(;estart != 0; estart--)
			if(entry->filename[estart] == '.')
				break;

		bool found = false;
		int slot = 0;
		for(; slot < ftFilled; slot++)
		{
			if(strncmp(filetypes[slot].extension, entry->filename +estart, 9) == 0)
			{ found = true; break; }
		}
		if(!found)
		{
			strcpy(filetypes[ftFilled].extension, entry->filename + estart);
			filetypes[ftFilled].filetype = entry->filetype;
			ftFilled++;
		}
	}

#endif

	void packBBA6(char *folderName)
	{
		bool isBBA = false;
		uint32_t msStart = GetTickCount();
		printf("Command: Packing folder to archive\n\n");
		DirStructEntry *lastElement = 0;
		DirStructEntry root;
		int cnt = 1;
		int allStringSize = 4;
		int allFileSize = 0;

		int dotPos = strlen(folderName) - 9;			//remove "unpacked"
		folderName[dotPos] = 0;
		isBBA = strncmp("bba", folderName + dotPos - 3, 3) == 0;
		FILE *writeHandle = fopen(folderName, "wb");
		if (!writeHandle)
			fatal(" ! Output file couldn't be created!\n");

		folderName[dotPos] = '.';
		_chdir(folderName);

		printf(" - Reading folder contents\n");
		ReadRootFolder(&root, ".",  &cnt, &allStringSize, &lastElement);

		fseek(writeHandle, sizeof(BBAHeader) + sizeof(S6ArcHeader), SEEK_SET);

		printf(" - Writing file contents: Element ");
		COORD curPos = getCurPos();
		DirStructEntry *elm = &root;
		int fileNum = 0;
		int fileBlockSize = 0;
		do
		{
			fileNum++;
			gotoxy(curPos);
			printf("%d/%d (%d%%)", fileNum, cnt, (100 * fileNum) / cnt);

			if (elm->type != ELM_DIR)
			{
				fgetpos(writeHandle, (fpos_t*)&elm->offset);
				FILE *readHandle = fopen(elm->path, "rb");
				if (!readHandle)
					fatal(" ! Couldn't read '%s'!\n", elm->path);

				int fileSize = 0;
				if (elm->type == ELM_UNCOMPRESSED)
				{
					int bytesRead;
					int runningCRC = 0;
					do
					{
						bytesRead = fread(copyBuf, 1, BUFSZ, readHandle);
						runningCRC = crc32(runningCRC, (const Bytef *)copyBuf, bytesRead);
						fwrite(copyBuf, 1, bytesRead, writeHandle);
						fileSize += bytesRead;
					} while (bytesRead == BUFSZ);
					elm->decompressedSize = fileSize;
					elm->decompressedCRC32 = runningCRC;
				}
				else
				{
					S6FileHeader cmpHeader;
					uint32_t dummyDir[4] = { 0, 0, 0, 0 };
					uint32_t crc32_in = 0;
					uint32_t crc32_out = 0;

					fseek(writeHandle, 8, SEEK_CUR);
					def(readHandle, writeHandle, Z_DEFAULT_COMPRESSION, &cmpHeader, &crc32_in, &crc32_out);
					
					elm->decompressedSize = dummyDir[2] = cmpHeader.decompressedSize;
					elm->decompressedCRC32 = dummyDir[3] = crc32_in;
					elm->compressedSize = cmpHeader.compressedSize + 8;
					fileSize = elm->decompressedSize;

					S6FileHeader_Encrypt(&cmpHeader, (S6DirEntry *)dummyDir);
					uint32_t fileHeaderCRC = crc32(0, (const Bytef *)&cmpHeader, sizeof(S6FileHeader));
					elm->compressedCRC32 = crc32_combine(fileHeaderCRC, crc32_out, elm->compressedSize - 16);

					fseek(writeHandle, elm->offset, SEEK_SET);
					fwrite(&cmpHeader, sizeof(S6FileHeader), 1, writeHandle);
					fseek(writeHandle, elm->compressedSize - 16, SEEK_CUR);
				}
				allFileSize += fileSize;

				fclose(readHandle);
			}
		} while (elm = elm->nextLinear);



		printf("\n - Creating directory\n");
		uint32_t hashTableSize = htSize(cnt);
		uint32_t htMemory = 4 + sizeof(S6HTEntry) * hashTableSize;
		int uncompressedDirSize = 
			sizeof(S6Directory) - sizeof(S6DirEntry) +				//directory header
			(sizeof(S6DirEntry)-4) * cnt + allStringSize +			//directory entries
			htMemory;												//hashtable

		S6Directory *dirObj = (S6Directory *)calloc(uncompressedDirSize, 1);
		S6_InitDir(dirObj);
		dirObj->dirEncrID = isBBA ? S6_BBA_CRYPTID : S6_MAP_CRYPTID; 
		dirObj->entriesCount = cnt;
		dirObj->hashtableOffset = uncompressedDirSize - htMemory;

		uint8_t *directory = (uint8_t *) dirObj->entries;

		elm = lastElement ? lastElement : &root;
		S6DirEntry *last = 0;
		int diroffset = 0;
		do
		{
			S6DirEntry *entry = (S6DirEntry*)(directory + diroffset);
			entry->filetype = elm->type;
			if (elm->type == ELM_DIR)
			{
				//entry->offset = 0;
				//entry->decompressedSize = 0; //calloc ;)
			}
			else
			{
				entry->offset = elm->offset;
				entry->decompressedSize = elm->decompressedSize;
				entry->compressedSize = elm->compressedSize;
				entry->compressedCRC32 = elm->compressedCRC32;
				entry->CRC32 = elm->decompressedCRC32;
			}
			entry->timestamp = 0;

			int dirpart = 0;
			int i = 0;
			for (; elm->path[i]; i++)
			{
				if (elm->path[i] == '\\')
					dirpart = i + 1;

				entry->filename[i] = elm->path[i];
			}
			entry->dirPart = dirpart;
			entry->filenameLength = i;

			int padding = 4 - (entry->filenameLength % 4); // [sic]
			int sizeOfThisEntry = sizeof(S6DirEntry)-4 + entry->filenameLength + padding;

			entry->firstChild = elm->firstChild ? elm->firstChild->dirOffset : -1;
			entry->nextSibling = elm->nextSibling ? elm->nextSibling->dirOffset : -1;
			last = entry;
			elm->dirOffset = diroffset;
			diroffset += sizeOfThisEntry;
		} while (elm = elm->prevLinear);

		printf(" - Creating hashtable\n");
		uint32_t mask = hashTableSize - 1;
		S6HashTable *hashTable = (S6HashTable *)((uint32_t)dirObj + dirObj->hashtableOffset);
		hashTable->entriesCount = hashTableSize;

		elm = &root;
		do
		{
			uint32_t hash = crc32(0, (const Bytef *)elm->path, strlen(elm->path));
			uint32_t masked = mask & hash;
			while (hashTable->entries[masked].offset)
				masked = (masked + 1) & mask;
			hashTable->entries[masked].CRC32 = hash;
			hashTable->entries[masked].offset = elm->dirOffset;

		} while (elm = elm->nextLinear);

		
		printf(" - Writing metadata\n");

		S6ArcHeader s6ArchiveHeader;
		memset(&s6ArchiveHeader, 0, sizeof(S6ArcHeader));
		s6ArchiveHeader.dirEncrID = dirObj->dirEncrID;
		s6ArchiveHeader.unknown1 = 0xD1C81BB5;
		fgetpos(writeHandle, (fpos_t *)&s6ArchiveHeader.directoryOffset);

		uint32_t compressedSize = uncompressedDirSize;
		uint32_t cryptContainerSize = uncompressedDirSize + 8; // +8 compression header
		uint8_t *compressedDir = (uint8_t *)calloc(cryptContainerSize + 4, 1); // +4 for padding
		compress2(compressedDir + 8, (uLongf *)&compressedSize, (const Bytef *)dirObj, uncompressedDirSize, Z_BEST_COMPRESSION);
		((CompressedData *)compressedDir)->compressedSize = compressedSize;
		((CompressedData *)compressedDir)->decompressedSize = uncompressedDirSize;

		int cryptPadding = (compressedSize + 8) % 4;
		if (cryptPadding)
			cryptPadding = 4 - cryptPadding;
		
		s6ArchiveHeader.directoryLength = compressedSize + 8 + cryptPadding;
		S6_Encrypt((uint32_t *)compressedDir, s6ArchiveHeader.directoryLength, s6ArchiveHeader.dirEncrID);
		
		s6ArchiveHeader.CRC32 = crc32(0, (const Bytef *)compressedDir, s6ArchiveHeader.directoryLength);

		fwrite(compressedDir, s6ArchiveHeader.directoryLength, 1, writeHandle);
		fseek(writeHandle, 0, SEEK_SET);

		BBAHeader bbaHeader;
		S6_InitBBAHeader(&bbaHeader);
		fwrite(&bbaHeader, sizeof(BBAHeader), 1, writeHandle);
		S6_Encrypt(&s6ArchiveHeader, sizeof(S6ArcHeader), bbaHeader.headerEncrID);
		fwrite(&s6ArchiveHeader, sizeof(S6ArcHeader), 1, writeHandle);
		fclose(writeHandle);

		double mbps = (1000.0 / (1024.0*1024.0))*((double)allFileSize / (double)(GetTickCount() - msStart));
		printf("\n - Done! (%3.2f MiB/s)\n", mbps > 0 ? mbps : 0);

		return;
	}

	void unpackBBA6(char *filename)
	{
		uint32_t msStart = GetTickCount();
		printf("Command: Unpacking archive to folder\n\n");

		FILE *handle = fopen(filename, "rb");
		if (!handle)
			fatal(" ! Couldn't read input file!");
	
		printf(" - Reading header\n");
		BBAHeader bbaHeader;
		fread(&bbaHeader, sizeof(BBAHeader), 1, handle);
		if(strncmp("BAF", bbaHeader.archiveHeader, 3))
			fatal(" ! Not a BBA archive!");
		if(bbaHeader.archiveVersion != 4)
			fatal(" ! BBA version %d not supported!", bbaHeader.archiveVersion);

		printf(" - Reading directory\n");

		S6ArcHeader arcHeader;
		fread(&arcHeader, sizeof(S6ArcHeader), 1, handle);
		if(!S6_Decrypt(&arcHeader, sizeof(S6ArcHeader), bbaHeader.headerEncrID))
			fatal(" ! error decrypting 0x%x!", bbaHeader.headerEncrID);

		CompressedData *cDict = (CompressedData *) malloc(arcHeader.directoryLength);

		fseek(handle, arcHeader.directoryOffset, SEEK_SET);
		fread(cDict, arcHeader.directoryLength, 1, handle);

		if(arcHeader.CRC32 != crc32(0, (const Bytef *) cDict, arcHeader.directoryLength))
			fatal(" ! dict crc32 wrong!");

		if(!S6_Decrypt(cDict, arcHeader.directoryLength, arcHeader.dirEncrID))
			fatal(" ! error decrypting 0x%x!", arcHeader.dirEncrID);

		S6Directory *dict = (S6Directory *) DecompressData(cDict);
		if(dict == 0)
			fatal(" ! error unpackinging dictionary!");

		//FILE* dd = fopen("../dirdump.bin", "wb");
		//fwrite(dict, cDict->decompressedSize, 1, dd);
		//fclose(dd);

		char *path;
		path = (char *)malloc(strlen(filename) + 10);
		path[0] = 0;
		strncat(path, filename, 300);
		strncat(path, ".unpacked", 300);

		_mkdir(path);
		_chdir(path);

		printf(" - Unpacking files: ");

		printf("Element ");
		COORD curPos = getCurPos();

		int allFileSize = 0;
		int dirOffset = 0;
		for (uint32_t i = 0; i < dict->entriesCount; i++)
		{
			gotoxy(curPos);
			printf("%d/%d (%d%%)", i+1, dict->entriesCount, (100 * (i+1)) / dict->entriesCount);

			S6DirEntry *entry;
			entry = (S6DirEntry *)((int)dict->entries + dirOffset);

			if (entry->filetype == 256)
				mkpath(entry->filename);
			else
			{
				S6_PrepareFolder(entry);

				FILE *writeHandle = fopen(entry->filename, "wb");
				if (!writeHandle)
					fatal(" ! Error: Couldn't write to '%s'!\n", entry->filename);

				uint32_t hash = crc32(0, (const Bytef *)entry->filename, entry->filenameLength);

				allFileSize += entry->decompressedSize;
				S6_UnpackFile(entry, handle, writeHandle);
				fclose(writeHandle);
			}
			int padding = 4 - (entry->filenameLength % 4);
			dirOffset += sizeof(S6DirEntry)-4 + entry->filenameLength + padding;
		}

		double mbps = (1000.0 / (1024.0*1024.0))*((double)allFileSize / (double)(GetTickCount() - msStart));
		printf("\n\n - Done! (%3.2f MiB/s)\n", mbps > 0 ? mbps: 0);
		return;
	}

	void main(int argc, char* argv[])
	{
		CCrashHandler ch;
		ch.SetProcessExceptionHandlers();
		ch.SetThreadExceptionHandlers();

		conHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		printf("\t\t\tbba6Tool by yoq\nVersion: v0.1\n\n");

		if (argc == 1)
		{
			printf("Usage: Drop a file or folder onto the program file!\n\n");

			getchar();
			return;
		}

		ShowConsoleCursor(false);

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;

		hFind = FindFirstFileExA(argv[1], FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0);

		bool noProblems = true;

		if (hFind == INVALID_HANDLE_VALUE)
		{
			printf("Error reading %s: (%d)\n", argv[1], GetLastError());
			getchar();
			getchar();
			return;
		}
		else
		{
			char *outputPath = 0;
			if(argc > 2)
				outputPath = argv[2];
			char *extractDir = 0;
			if(argc > 3)
				extractDir = argv[3];

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				packBBA6(argv[1]);
			else
				unpackBBA6(argv[1]);
		}

		FindClose(hFind);
	}

}