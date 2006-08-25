#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BinaryOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStreamBase
//
//		Base class for write streams that store DWORD-sized
//		data in a space efficient format. CBinaryOutStreamBase
//		is used as a base class for stream data handling.
//
//		DWORDs are stored as follows:
//
//		* a sequence of 1 .. 5 bytes in 7/8 codes
//		  (i.e. 7 data bits per byte)
//		* the highest bit is used to indicate whether this
//		  is the last byte of the value (0) or if more bytes
//		  follow (1)
//		* the data is stored in little-endian order
//		  (i.e the first byte contains bits 0..6, the next
//		   byte 7 .. 13 and so on)
//
///////////////////////////////////////////////////////////////

class CPackedDWORDOutStreamBase : public CBinaryOutStreamBase
{
protected:

	// add data to the stream

	void Add (DWORD value);

public:

	// construction / destruction: nothing special to do

	CPackedDWORDOutStreamBase ( CCacheFileOutBuffer* aBuffer
						      , SUB_STREAM_ID anID);
	virtual ~CPackedDWORDOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStream
//
//		instantiable sub-class of CPackedDWORDOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedDWORDOutStream 
	: public COutStreamImplBase< CPackedDWORDOutStream
							   , CPackedDWORDOutStreamBase
		                       , PACKED_DWORD_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CPackedDWORDOutStream
							  , CPackedDWORDOutStreamBase
							  , PACKED_DWORD_STREAM_TYPE_ID> TBase;

	typedef DWORD value_type;

	// construction / destruction: nothing special to do

	CPackedDWORDOutStream ( CCacheFileOutBuffer* aBuffer
						  , SUB_STREAM_ID anID);
	virtual ~CPackedDWORDOutStream() {};

	// public Add() methods

	using TBase::Add;
};

///////////////////////////////////////////////////////////////
//
// operator<< 
//
//		for CPackedDWORDOutStreamBase derived streams and vectors.
//
///////////////////////////////////////////////////////////////

template<class S, class V>
S& operator<< (S& stream, const std::vector<V>& data)
{
	typedef typename std::vector<V>::const_iterator IT;

	stream.Add ((typename S::value_type)data.size());
	for (IT iter = data.begin(), end = data.end(); iter != end; ++iter)
		stream.Add ((typename S::value_type)(*iter));

	return stream;
}
