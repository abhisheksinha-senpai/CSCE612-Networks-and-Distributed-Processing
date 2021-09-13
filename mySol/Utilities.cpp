#include "pch.h"
#include "Socket.h"

char* parseFile(char* fileName)
{
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	// process errors
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("\t  CreateFile failed with %d\n", GetLastError());
		return  NULL;
	}

	LARGE_INTEGER li;
	BOOL bRet = GetFileSizeEx(hFile, &li);
	// process errors
	if (bRet == 0)
	{
		printf("\t  GetFileSizeEx error %d\n", GetLastError());
		return NULL;
	}

	// read file into a buffer
	int fileSize = (DWORD)li.QuadPart;			// assumes file size is below 2GB; otherwise, an __int64 is needed
	DWORD bytesRead;
	// allocate buffer
	char* fileBuf = new char[fileSize];
	// read into the buffer
	bRet = ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL);

	// process errors
	if (bRet == 0 || bytesRead != fileSize)
	{
		printf("\t  ReadFile failed with %d\n", GetLastError());
		return NULL;
	}

	printf("Opened %s with size %d\n",fileName, fileSize);
	// done with the file
	CloseHandle(hFile);
	return fileBuf;
}

char* parseFile_gets(char* fileName)
{
	FILE* fptr;
	fptr = fopen(fileName, "rb");
	char URL_from_file[MAX_URL_LEN];
	if (fptr == NULL)
	{
		printf("Unable to open file\n");
		return NULL;
	}
	else
	{
		while (fscanf(fptr, "%[^\n] ", &URL_from_file) != EOF)
		{
			printf("> %s\n", URL_from_file);
		}
	}

	return NULL;
}