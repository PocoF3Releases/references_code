#ifndef _F2FS_DEDUP_H_
#define _F2FS_DEDUP_H_


#include "MD5Sum.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <stdbool.h>

struct md5Data {
	int errCode;
	char hash[MD5SUM_STRING_LEN + 1];
};

struct md5Data computeFileMD5(const char *file_name, bool log);
int createHardLink(const char *sourcePath, const char *destPath, bool log);
int restoreLinkFile(const char *destPath, bool log);
int copyFile(const char *pathName, const char *nickName);
bool isNickFile(const char *file_name);
unsigned int getChmod(const char* pathName);
void timeLog(const char * title, bool flag);
void printLog(const char* title, const char* str1, const char* str2, int errCode, bool log);


#endif
