#pragma once

// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BLOBOutStream.h"

///////////////////////////////////////////////////////////////
//
// CHuffmanOutStream
//
//		Base class for write streams that contain a single 
//		binary chunk of data.
//
//		Add() must be called at most once. Since it will close
//		the stream implicitly, there is no need to buffer the
//		data within the stream. It will be written directly
//		to the file buffer instead.
//
//		That means, all sub-streams will be written as well!
//
///////////////////////////////////////////////////////////////

class CHuffmanOutStreamBase : public CBLOBOutStreamBase
{
private:

	// per-character info

	typedef unsigned __int64 QWORD;
	typedef unsigned char BYTE;

#ifdef _WIN64
	typedef QWORD key_type;
	typedef QWORD count_block_type;
	typedef DWORD encode_block_type;
#else
	typedef DWORD key_type;
	typedef DWORD count_block_type;
	typedef WORD encode_block_type;
#endif

	enum 
	{
		BUCKET_COUNT = 1 << (8 * sizeof (BYTE)),  // we have to encode up to 2^8 byte different values
		KEY_BITS = sizeof (key_type) * 8,
		MAX_ENCODING_LENGTH = (sizeof (DWORD) - 1) * 8 / 2	// 2 encoded values plus odd bits shall 
										    // still fit into a single DWORD
	};

	key_type key[BUCKET_COUNT];
	BYTE keyLength[BUCKET_COUNT];
	DWORD count[BUCKET_COUNT];

	BYTE sorted [BUCKET_COUNT];
	size_t sortedCount;

	// huffman encoding stages:

	// (1) determine distribution

	void CountValues ( const unsigned char* source
					 , const unsigned char* end);

	// (2) prepare for key assignment: sort by frequency

	void SortByFrequency();

	// (3) recursively construct & assign keys

	void AssignEncoding ( BYTE* first
					    , BYTE* last
					    , key_type encoding
					    , BYTE bitCount);

	// (4) target buffer size calculation

	DWORD CalculatePackedSize();

	// (5) target buffer size calculation

	void WriteHuffmanTable (BYTE*& dest);

	// (6) write encoded target stream

	void WriteHuffmanEncoded ( const BYTE* source
						     , const BYTE* end
							 , BYTE* dest);

public:

	// construction / destruction: nothing special to do

	CHuffmanOutStreamBase ( CCacheFileOutBuffer* aBuffer
						  , SUB_STREAM_ID anID);
	virtual ~CHuffmanOutStreamBase() {};

	// write local stream data and close the stream

	void Add (const unsigned char* source, size_t byteCount);
};

///////////////////////////////////////////////////////////////
//
// CHuffmanOutStream
//
//		instantiable sub-class of CHuffmanOutStreamBase.
//
///////////////////////////////////////////////////////////////

template COutStreamImpl< CHuffmanOutStreamBase
					   , HUFFMAN_STREAM_TYPE_ID>;
typedef COutStreamImpl< CHuffmanOutStreamBase
					  , HUFFMAN_STREAM_TYPE_ID> CHuffmanOutStream;
