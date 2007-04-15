#pragma once
#include "logiteratorbase.h"

class CCopyFollowingLogIterator :
	public CLogIteratorBase
{
protected:

	// implement copy-following and termination-on-delete

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CCopyFollowingLogIterator ( const CCachedLogInfo* cachedLog
							  , size_t startRevision
							  , const CDictionaryBasedPath& startPath);
	virtual ~CCopyFollowingLogIterator(void);
};
