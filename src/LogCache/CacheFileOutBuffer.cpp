#include "StdAfx.h"
#include "CacheFileOutBuffer.h"

// write buffer content to disk

void CCacheFileOutBuffer::Flush()
{
	if (used > 0)
	{
		DWORD written = 0;
		WriteFile (file, buffer.get(), used, &written, NULL);
		used = 0;
	}
}

// construction / destruction: auto- open/close

CCacheFileOutBuffer::CCacheFileOutBuffer (const std::wstring& fileName)
	: file (INVALID_HANDLE_VALUE)
	, buffer (new unsigned char [BUFFER_SIZE])
	, used (0)
	, fileSize (0)
	, streamIsOpen (false)
{
	file = CreateFile ( fileName.c_str()
					  , GENERIC_WRITE
					  , 0
					  , NULL
					  , CREATE_ALWAYS
					  , FILE_ATTRIBUTE_NORMAL
					  , NULL);
	if (file == INVALID_HANDLE_VALUE)
		throw std::exception ("can't create log cache file");

	// the version id

	Add (OUR_LOG_CACHE_FILE_VERISON);
	Add (MIN_LOG_CACHE_FILE_VERISON);
}

CCacheFileOutBuffer::~CCacheFileOutBuffer()
{
	if (file != INVALID_HANDLE_VALUE)
	{
		streamOffsets.push_back (fileSize);

		size_t lastOffset = streamOffsets[0];
		for (size_t i = 1, count = streamOffsets.size(); i < count; ++i)
		{
			size_t offset = streamOffsets[i];
			size_t size = offset - lastOffset;

			if (size >= (DWORD)(-1))
				throw std::exception ("stream too large");

			Add ((DWORD)size);
			lastOffset = offset;
		}

		Add ((DWORD)(streamOffsets.size()-1));

		Flush();

		CloseHandle (file);
	}
}

// write data to file

STREAM_INDEX CCacheFileOutBuffer::OpenStream()
{
	// don't overflow (2^32 streams is quite unlikely, though)

	assert ((STREAM_INDEX)streamOffsets.size() != (-1));

	assert (streamIsOpen == false);
	streamIsOpen = true;

	streamOffsets.push_back (fileSize);
	return (STREAM_INDEX)streamOffsets.size()-1;
}

void CCacheFileOutBuffer::Add (const unsigned char* data, DWORD bytes)
{
	// test for buffer overflow

	if (used + bytes > BUFFER_SIZE)
	{
		Flush();

		// don't buffer large chunks of data

		if (bytes >= BUFFER_SIZE)
		{
			DWORD written = 0;
			WriteFile (file, data, bytes, &written, NULL);
			fileSize += written;

			return;
		}
	}

	// buffer small chunks of data

	memcpy (buffer.get() + used, data, bytes);

	used += bytes;
	fileSize += bytes;
}

void CCacheFileOutBuffer::CloseStream()
{
	assert (streamIsOpen == true);
	streamIsOpen = false;
}


