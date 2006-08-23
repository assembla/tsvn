#pragma once

///////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////

#include "StreamFactory.h"
#include "StreamTypeIDs.h"
#include "CacheFileInBuffer.h"

///////////////////////////////////////////////////////////////
//
// IHierarchicalInStream
//
//		the generic read stream interface. 
//		Streams form a tree.
//
///////////////////////////////////////////////////////////////

class IHierarchicalInStream
{
public:

	// add a sub-stream

	virtual IHierarchicalInStream* GetSubStream (SUB_STREAM_ID subStreamID) = 0;
};

///////////////////////////////////////////////////////////////
//
// CHierachicalInStreamBase
//
//		implements IHierarchicalInStream except for GetTypeID().
//		It mainly manages the sub-streams. 
//
//		For details of the stream format see CHierachicalOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CHierachicalInStreamBase : public IHierarchicalInStream
{
private:

	// the sub-streams (in no particular order)

	typedef std::map<SUB_STREAM_ID, IHierarchicalInStream*> TSubStreams;
	TSubStreams subStreams;

	// stream content
	// (sub-stream info will be excluded after ReadSubStreams()) 

	const unsigned char* first;
	const unsigned char* last;

protected:

	// for usage with CRootInStream

	CHierachicalInStreamBase();

	void ReadSubStreams ( CCacheFileInBuffer* buffer
				        , STREAM_INDEX index);

public:

	// construction / destruction

	CHierachicalInStreamBase ( CCacheFileInBuffer* buffer
							 , STREAM_INDEX index);
	virtual ~CHierachicalInStreamBase(void);

	// implement IHierarchicalOutStream

	virtual IHierarchicalInStream* GetSubStream (SUB_STREAM_ID subStreamID);
};
