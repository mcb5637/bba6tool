#include "stdafx.h"
#include "treeReader.h"
#include "tinydir.h"
#include "s6data.h"

DirStructEntry *readFolder(char *dirname, int *counter, int *strLenCnt, DirStructEntry **lastElm)
{
	tinydir_dir dir;
	tinydir_open(&dir, dirname);
	DirStructEntry* firstEntry = 0;
	DirStructEntry* lastElement = 0;
	DirStructEntry *lastLinear = 0;
	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		if (file.name[0] != '.')
		{
			(*counter)++;
			DirStructEntry *entry;
			entry = (DirStructEntry*)calloc(sizeof(DirStructEntry)-2 + strlen(file.path) + 1, 1);

			if (lastLinear)
			{
				lastLinear->nextLinear = entry;
				entry->prevLinear = lastLinear;
			}

			if (file.is_dir)
			{
				entry->firstChild = readFolder(file.path, counter, strLenCnt, &lastLinear);

				if (entry->firstChild)
				{
					entry->nextLinear = entry->firstChild;
					entry->firstChild->prevLinear = entry;
				}
				else
					lastLinear = entry;
			}
			else
			{
				entry->firstChild = 0;
				entry->nextLinear = 0;
				lastLinear = entry;
			}



			entry->type = file.is_dir ? ELM_DIR : ELM_UNCOMPRESSED;

			int len = 0;
			int fileExt = 0;
			for (; char c = file.path[2 + len]; len++)
			{
				entry->path[len] = c == '/' ? '\\' : tolower(c);
				if (c == '.')
					fileExt = len + 1;
			}

			entry->path[len] = 0;

			if (!file.is_dir)
			{
				if (!strcmp("xml", &entry->path[fileExt]) ||
					!strcmp("bin", &entry->path[fileExt]) ||
					!strcmp("fdb", &entry->path[fileExt]) ||
					!strcmp("lua", &entry->path[fileExt]) ||
					!strcmp("fx", &entry->path[fileExt]) ||
					!strcmp("anm", &entry->path[fileExt]) ||
					!strcmp("dff", &entry->path[fileExt]) ||
					!strcmp("spt", &entry->path[fileExt]) ||
					!strcmp("dds", &entry->path[fileExt]) ||
					!strcmp("txt", &entry->path[fileExt]) ||
					!strcmp("cs", &entry->path[fileExt]))
					entry->type = ELM_COMPRESSED;
			}

			int pathLen = len;
			int padding = 4 - (pathLen % 4);
			(*strLenCnt) += pathLen + padding;

			if (firstEntry == 0)
				firstEntry = entry;
			else
				lastElement->nextSibling = entry;

			lastElement = entry;
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);

	if (lastElement)
		lastElement->nextSibling = 0;

	*lastElm = lastLinear;

	return firstEntry;
}

DirStructEntry *ReadRootFolder(DirStructEntry *root, char *dirname, int *counter, int *fileNameCounter, DirStructEntry **lastElement)
{
	root->type = ELM_DIR;
	root->path[0] = '.';
	root->path[1] = 0;
	root->nextSibling = 0;
	root->nextLinear = 0;
	root->prevLinear = 0;
	root->firstChild = readFolder(dirname, counter, fileNameCounter, lastElement);
	if (root->firstChild)
	{
		root->nextLinear = root->firstChild;
		root->firstChild->prevLinear = root;
		root->prevLinear = 0;
	}
	return root;
}