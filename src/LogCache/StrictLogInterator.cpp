#include "StdAfx.h"
#include "StrictLogInterator.h"

// begin namespace LogCache

namespace LogCache
{

// implement as no-op

bool CStrictLogInterator::HandleCopyAndDelete()
{
	return false;
}

// construction / destruction 
// (nothing special to do)

CStrictLogInterator::CStrictLogInterator ( const CCachedLogInfo* cachedLog
										 , revision_t startRevision
										 , const CDictionaryBasedPath& startPath)
	: CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CStrictLogInterator::~CStrictLogInterator(void)
{
}

// end namespace LogCache

}
