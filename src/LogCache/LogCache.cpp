// LogCache.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RootInStream.h"
#include "RootOutStream.h"
#include "StringDictonary.h"

void ReadStream (const std::wstring& fileName)
{
	CRootInStream stream (fileName);

	CStringDictonary dictionary;

	stream >> dictionary;
}

void WriteStream (const std::wstring& fileName)
{
	CRootOutStream stream (fileName);

	CStringDictonary dictionary;
	dictionary.insert ("test");
	dictionary.insert ("hugo");
	dictionary.insert ("otto");

	stream << dictionary;
}

int _tmain(int argc, _TCHAR* argv[])
{
	WriteStream (L"C:\\temp\\test.stream");
	ReadStream (L"C:\\temp\\test.stream");

	return 0;
}

