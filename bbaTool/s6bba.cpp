#pragma pack(4)

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <direct.h>
#include <windows.h>
#include <varargs.h>
#include "d:/vclibs/zlib-1.2.8/zlib.h"
#include "shokCrypt.h"
#include "s6data.h"
#include "compress.h"

#define BUFSZ	4096



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
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), p);
	}

	COORD getCurPos()
	{
		_CONSOLE_SCREEN_BUFFER_INFO  curInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
		return curInfo.dwCursorPosition;
	}


