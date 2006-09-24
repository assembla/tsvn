#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBinaryOutStreamBase
//
//		Base class for write streams that contain arbitrary
//		binary data. All data gets buffered.
//
//		If the stream grows to large (x64 only), it will
//		throw an exception upon AutoClose().
//
///////////////////////////////////////////////////////////////

class CBinaryOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// data to write (may be NULL)

	std::vector<unsigned char> data;

protected:

	// write our data to the file

	virtual void WriteThisStream (CCacheFileOutBuffer* buffer);

	// add data to the stream

	void Add (const unsigned char* source, size_t byteCount)
	{
		data.insert (data.end(), source, source + byteCount);
	}

	void Add (unsigned char c)
	{
		data.push_back (c);
	};

public:

	// construction / destruction: nothing special to do

	CBinaryOutStreamBase ( CCacheFileOutBuffer* aBuffer
					     , SUB_STREAM_ID anID);
	virtual ~CBinaryOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CBinaryOutStream
//
//		instantiable sub-class of CBinaryOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CBinaryOutStream : public COutStreamImplBase< CBinaryOutStream
												  , CBinaryOutStreamBase
					                              , BINARY_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CBinaryOutStream
							  , CBinaryOutStreamBase
							  , BINARY_STREAM_TYPE_ID> TBase;

	// construction / destruction: nothing special to do

	CBinaryOutStream ( CCacheFileOutBuffer* aBuffer
					 , SUB_STREAM_ID anID);
	virtual ~CBinaryOutStream() {};

	// public Add() methods

	using TBase::Add;
};
