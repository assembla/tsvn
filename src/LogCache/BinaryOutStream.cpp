#include "StdAfx.h"
#include "BinaryOutStream.h"

///////////////////////////////////////////////////////////////
//
// CBinaryOutStreamBase
//
///////////////////////////////////////////////////////////////

// write our data to the file

void CBinaryOutStreamBase::WriteThisStream (CCacheFileOutBuffer* buffer)
{
	if (data.size() > (DWORD)(-1))
		throw std::exception ("binary stream too large");

	if (!data.empty())
		buffer->Add (&data.at(0), (DWORD)data.size());
}

// construction: nothing special to do

CBinaryOutStreamBase::CBinaryOutStreamBase ( CCacheFileOutBuffer* aBuffer
					                       , SUB_STREAM_ID anID)
	: CHierachicalOutStreamBase (aBuffer, anID)
{
}

///////////////////////////////////////////////////////////////
//
// CBinaryOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CBinaryOutStream::CBinaryOutStream ( CCacheFileOutBuffer* aBuffer
								   , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
