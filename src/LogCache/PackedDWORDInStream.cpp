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
	, lastValue (0)
	, count (0)
{
}

// data access

DWORD CPackedDWORDInStreamBase::InternalGetValue()
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

DWORD CPackedDWORDInStreamBase::GetValue()
{
	while (true)
	{
		if (count > 0)
		{
			--count;
			return lastValue;
		}
		else
		{
			DWORD result = InternalGetValue();
			if (result != 0)
				return result-1;

			count = InternalGetValue();
			lastValue = InternalGetValue();
		}
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
