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

	DWORD lastValue;
	DWORD count;

	// not ment to be instantiated

	// construction: nothing to do here

	CPackedDWORDInStreamBase ( CCacheFileInBuffer* buffer
						     , STREAM_INDEX index);

	// data access

	DWORD InternalGetValue();
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

	typedef DWORD value_type;

	// construction / destruction: nothing special to do

	CPackedDWORDInStream ( CCacheFileInBuffer* buffer
					     , STREAM_INDEX index);
	virtual ~CPackedDWORDInStream() {};

	// public data access methods

	using TBase::GetValue;
};

///////////////////////////////////////////////////////////////
//
// operator>> 
//
//		for CPackedDWORDInStreamBase derived streams and vectors.
//
///////////////////////////////////////////////////////////////

template<class S, class V>
S& operator>> (S& stream, std::vector<V>& data)
{
	typedef typename std::vector<V>::const_iterator IT;

	size_t count = (size_t)stream.GetValue();

	data.clear();
	data.reserve (count);

	for (; count > 0; --count)
		data.push_back ((V)stream.GetValue());

	return stream;
}
