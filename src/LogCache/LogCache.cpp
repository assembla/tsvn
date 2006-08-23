// LogCache.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RootInStream.h"
#include "RootOutStream.h"

void ReadStream (const std::wstring& fileName)
{
	CRootInStream stream (fileName);
}

void WriteStream (const std::wstring& fileName)
{
	CRootOutStream stream (fileName);
}

int _tmain(int argc, _TCHAR* argv[])
{
	WriteStream (L"C:\\temp\\test.stream");
	ReadStream (L"C:\\temp\\test.stream");

	return 0;
}

