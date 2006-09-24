#include "StdAfx.h"
#include "DiffIntegerInStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CDiffIntegerInStreamBase::CDiffIntegerInStreamBase (CCacheFileInBuffer* buffer
												   , STREAM_INDEX index)
	: CPackedIntegerInStreamBase (buffer, index)
	, lastValue (0)
{
}

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffIntegerInStream::CDiffIntegerInStream ( CCacheFileInBuffer* buffer
										   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
