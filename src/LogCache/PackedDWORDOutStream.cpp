#include "StdAfx.h"
#include "PackedDWORDOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStreamBase
//
///////////////////////////////////////////////////////////////

// add data to the stream

void CPackedDWORDOutStreamBase::Add (DWORD value)
{
	while (true)
	{
		if (value < 0x80)
		{
			CBinaryOutStreamBase::Add ((unsigned char)value);
			return;
		}
		
		CBinaryOutStreamBase::Add ((unsigned char)(value & 0x7f) + 0x80);
		value >>= 7;
	}
}

// construction: nothing special to do

CPackedDWORDOutStreamBase::CPackedDWORDOutStreamBase ( CCacheFileOutBuffer* aBuffer
													 , SUB_STREAM_ID anID)
	: CBinaryOutStreamBase (aBuffer, anID)
{
}

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedDWORDOutStream::CPackedDWORDOutStream ( CCacheFileOutBuffer* aBuffer
										     , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
