#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "PackedIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStreamBase
//
//		enhances CPackedIntegerOutStreamBase to store the
//		integer values differentially, i.e. the difference
//		to the previous value is stored (in packed signed
//		format). The base value for the first element is 0.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerOutStreamBase : public CPackedIntegerOutStreamBase
{
private:

	// last value written (i.e. our diff base)

	int lastValue;

protected:

	// add data to the stream

	void Add (int value)
	{
		CPackedIntegerOutStreamBase::Add (value - lastValue);
		lastValue = value;
	}

public:

	// construction / destruction: nothing special to do

	CDiffIntegerOutStreamBase ( CCacheFileOutBuffer* aBuffer
						      , SUB_STREAM_ID anID);
	virtual ~CDiffIntegerOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStream
//
//		instantiable sub-class of CDiffIntegerOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerOutStream 
	: public COutStreamImplBase< CDiffIntegerOutStream
							   , CDiffIntegerOutStreamBase
		                       , DIFF_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CDiffIntegerOutStream
							  , CDiffIntegerOutStreamBase
		                      , DIFF_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffIntegerOutStream ( CCacheFileOutBuffer* aBuffer
						  , SUB_STREAM_ID anID);
	virtual ~CDiffIntegerOutStream() {};

	// public Add() methods

	using TBase::Add;
};
