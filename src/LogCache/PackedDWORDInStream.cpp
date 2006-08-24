#include "StdAfx.h"
#include "PackedDWORDInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CPackedDWORDInStreamBase::CPackedDWORDInStreamBase (CCacheFileInBuffer* buffer
												   , STREAM_INDEX index)
	: CBinaryInStreamBase (buffer, index)
{
}

// data access

DWORD CPackedDWORDInStreamBase::GetValue()
{
	DWORD result = 0;
	char shift = 0;

	while (true)
	{
		DWORD c = GetByte();
		if (c < 0x80)
			return result + (c << shift);

		result += ((c - 0x80) << shift);
		shift += 7;
	}
}

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedDWORDInStream::CPackedDWORDInStream ( CCacheFileInBuffer* buffer
										   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
