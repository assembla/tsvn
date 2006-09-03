#include "StdAfx.h"
#include "CacheFileInBuffer.h"

// construction utilities

void CMappedInFile::MapToMemory (const std::wstring& fileName)
{
    // create the file handle & open the file r/o

	file = CreateFile ( fileName.c_str()
					  , GENERIC_READ
					  , FILE_SHARE_READ
					  , NULL
					  , OPEN_EXISTING
					  , FILE_ATTRIBUTE_NORMAL
					  , NULL);
	if (file == INVALID_HANDLE_VALUE)
		throw std::exception ("can't read log cache file");

	// get buffer size (we can always cast safely from 
	// fileSize to size since the memory mapping will fail 
	// anyway for > 4G files on win32)
    
	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
    GetFileSizeEx (file, &fileSize);
	size = (size_t)fileSize.QuadPart;

    // create a file mapping object able to contain the whole file content

    mapping = CreateFileMapping (file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (mapping == INVALID_HANDLE_VALUE)
		throw std::exception ("can't create mapping for log cache file");

    // map the whole file

	LPVOID address = MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);
    if (address == NULL)
		throw std::exception ("can't map the log cache file into memory");

	// set our buffer pointer

	buffer = reinterpret_cast<const unsigned char*>(address);
}

// construction / destruction: auto- open/close

CMappedInFile::CMappedInFile (const std::wstring& fileName)
    : file (INVALID_HANDLE_VALUE)
	, mapping (INVALID_HANDLE_VALUE)
	, buffer (NULL)
	, size (0)
{
	MapToMemory (fileName);
}

CMappedInFile::~CMappedInFile()
{
	if (buffer != NULL)
		UnmapViewOfFile (buffer);

	if (mapping != INVALID_HANDLE_VALUE)
		CloseHandle (mapping);

	if (file != INVALID_HANDLE_VALUE)
		CloseHandle (file);
}

// construction utilities

void CCacheFileInBuffer::ReadStreamOffsets()
{
	// minimun size: 3 DWORDs

	if (GetSize() < 3 * sizeof (DWORD))
		throw std::exception ("log cache file too small");

	// extract version numbers

	DWORD creatorVersion = *GetDWORD (0);
	DWORD requiredVersion = *GetDWORD (sizeof (DWORD));

	// check version number

	if (requiredVersion > MAX_LOG_CACHE_FILE_VERISON)
		throw std::exception ("log cache file format too new");

    // number of streams in file

	DWORD streamCount = *GetDWORD (GetSize() - sizeof (DWORD));
	if (GetSize() < (3 + streamCount) * sizeof (DWORD))
		throw std::exception ("log cache file too small to hold stream directory");

	// read stream sizes and ranges list

	const unsigned char* lastStream = GetBuffer() + 2* sizeof (DWORD);

	streamContents.reserve (streamCount+1);
	streamContents.push_back (lastStream);

	size_t contentEnd = GetSize() - (streamCount+1) * sizeof (DWORD);
	const DWORD* streamSizes = GetDWORD (contentEnd);

	for (DWORD i = 0; i < streamCount; ++i)
	{
		lastStream += streamSizes[i];
		streamContents.push_back (lastStream);
	}

	// consistency check

	if (lastStream - GetBuffer() != contentEnd)
		throw std::exception ("stream directory corrupted");
}


// construction / destruction: auto- open/close

CCacheFileInBuffer::CCacheFileInBuffer (const std::wstring& fileName)
    : CMappedInFile (fileName)
{
	ReadStreamOffsets();
}

CCacheFileInBuffer::~CCacheFileInBuffer()
{
}

// access streams

STREAM_INDEX CCacheFileInBuffer::GetLastStream()
{
	return (STREAM_INDEX)(streamContents.size()-2);
}

void CCacheFileInBuffer::GetStreamBuffer ( STREAM_INDEX index
										 , const unsigned char* &first
										 , const unsigned char* &last)
{
	if ((size_t)index >= streamContents.size()-1)
		throw std::exception ("invalid stream index");

	first = streamContents[index];
	last = streamContents[index+1];
}