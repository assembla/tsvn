#pragma once
#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

class CStrictLogInterator :
	public CLogIteratorBase
{
protected:

	// implement as no-op

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CStrictLogInterator ( const CCachedLogInfo* cachedLog
						, revision_t startRevision
						, const CDictionaryBasedPath& startPath);
	virtual ~CStrictLogInterator(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

