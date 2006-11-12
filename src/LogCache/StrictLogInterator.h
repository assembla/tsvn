#pragma once
#include "logiteratorbase.h"

class CStrictLogInterator :
	public CLogIteratorBase
{
protected:

	// implement as no-op

	virtual void HandleCopy();

public:

	// construction / destruction 
	// (nothing special to do)

	CStrictLogInterator ( const CCachedLogInfo* cachedLog
						, size_t startRevision
						, const CDictionaryBasedPath& startPath);
	virtual ~CStrictLogInterator(void);
};
