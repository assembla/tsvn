#include "StdAfx.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffIntegerOutStreamBase::CDiffIntegerOutStreamBase ( CCacheFileOutBuffer* aBuffer
												     , SUB_STREAM_ID anID)
	: CPackedIntegerOutStreamBase (aBuffer, anID)
	, lastValue (0)
{
}

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffIntegerOutStream::CDiffIntegerOutStream ( CCacheFileOutBuffer* aBuffer
										     , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
