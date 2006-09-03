#pragma once

class CMappedInFile
{
private:

	// the file

	HANDLE file;

	// the memory mapping

	HANDLE mapping;

	// file content memory address

	const unsigned char* buffer;

	// physical file size (== file size)

	size_t size;

	// construction utilities

	void MapToMemory (const std::wstring& fileName);

public:

	// construction / destruction: auto- open/close

	CMappedInFile (const std::wstring& fileName);
	~CMappedInFile();

	// access streams

	const unsigned char* GetBuffer() const;
	size_t GetSize() const;
};

///////////////////////////////////////////////////////////////
// access streams
///////////////////////////////////////////////////////////////

inline const unsigned char* CMappedInFile::GetBuffer() const
{
	return buffer;
}

inline size_t CMappedInFile::GetSize() const
{
	return size;
}

///////////////////////////////////////////////////////////////
// index type used to address a certain stream within the file.
///////////////////////////////////////////////////////////////

typedef int STREAM_INDEX;

///////////////////////////////////////////////////////////////
// the largest file version number we will understand
///////////////////////////////////////////////////////////////

enum
{
	MAX_LOG_CACHE_FILE_VERISON = 0x20060701,
};

///////////////////////////////////////////////////////////////
//
// CCacheFileInBuffer
//
//		class that maps files created by CCacheFileOutBuffer
//		(see there for format details) into memory.
//
//		The file must be small enough to fit into your address
//		space (i.e. less than about 1 GB under win32).
//
//		GetLastStream() will return the index of the root stream.
//		Streams will be returned as half-open ranges [first, last).
//
///////////////////////////////////////////////////////////////

class CCacheFileInBuffer : private CMappedInFile
{
private:

	// start-addresses of all streams (i.e. streamCount + 1 entry)

	std::vector<const unsigned char*> streamContents;

	// construction utilities

	void ReadStreamOffsets();

	// data access utility

	const DWORD* GetDWORD (size_t offset) const
	{
		// ranges should have been checked before

		assert ((offset < GetSize()) && (offset + sizeof (DWORD) <= GetSize()));
		return reinterpret_cast<const DWORD*>(GetBuffer() + offset);
	}

public:

	// construction / destruction: auto- open/close

	CCacheFileInBuffer (const std::wstring& fileName);
	~CCacheFileInBuffer();

	// access streams

	STREAM_INDEX GetLastStream();
	void GetStreamBuffer ( STREAM_INDEX index
						 , const unsigned char* &first
						 , const unsigned char* &last);
};

