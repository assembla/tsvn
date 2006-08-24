#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BinaryInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStreamBase
//
//		Base class for all read streams containing packed 
//		integer data. See CPackedDWORDOutStreamBase for datails 
//		of the storage format.
//
///////////////////////////////////////////////////////////////

class CPackedDWORDInStreamBase : public CBinaryInStreamBase
{
protected:

	// not ment to be instantiated

	// construction: nothing to do here

	CPackedDWORDInStreamBase ( CCacheFileInBuffer* buffer
						     , STREAM_INDEX index);

	// data access

	DWORD GetValue();

public:

	// destruction

	virtual ~CPackedDWORDInStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStream
//
//		instantiable sub-class of CPackedDWORDInStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedDWORDInStream 
	: public CInStreamImplBase< CPackedDWORDInStream
							  , CPackedDWORDInStreamBase
							  , PACKED_DWORD_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CPackedDWORDInStream
							 , CPackedDWORDInStreamBase
							 , PACKED_DWORD_STREAM_TYPE_ID> TBase;

	// construction / destruction: nothing special to do

	CPackedDWORDInStream ( CCacheFileInBuffer* buffer
					     , STREAM_INDEX index);
	virtual ~CPackedDWORDInStream() {};

	// public data access methods

	using TBase::GetValue;
};
