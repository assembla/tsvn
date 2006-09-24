#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "PackedIntegerInStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStreamBase
//
//		enhances CPackedIntegerInStreamBase to store values 
//		differentially.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerInStreamBase : public CPackedIntegerInStreamBase
{
private:

	// our current diff base

	int lastValue;

protected:

	// not ment to be instantiated

	// construction: nothing to do here

	CDiffIntegerInStreamBase ( CCacheFileInBuffer* buffer
						     , STREAM_INDEX index);

	// data access

	int GetValue()
	{
		lastValue += CPackedIntegerInStreamBase::GetValue();
		return lastValue;
	}

public:

	// destruction

	virtual ~CDiffIntegerInStreamBase() {};

	// extend Reset() to reset lastValue properly

	virtual void Reset()
	{
		CPackedIntegerInStreamBase::Reset();
		lastValue = 0;
	}
};

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStream
//
//		instantiable sub-class of CDiffIntegerInStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerInStream 
	: public CInStreamImplBase< CDiffIntegerInStream
							  , CDiffIntegerInStreamBase
							  , DIFF_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CDiffIntegerInStream
							 , CDiffIntegerInStreamBase
							 , DIFF_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffIntegerInStream ( CCacheFileInBuffer* buffer
					     , STREAM_INDEX index);
	virtual ~CDiffIntegerInStream() {};

	// public data access methods

	using TBase::GetValue;
};
