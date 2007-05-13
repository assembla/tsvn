#include "StdAfx.h"
#include "StrictLogIterator.h"

// begin namespace LogCache

namespace LogCache
{

// implement as no-op

bool CStrictLogIterator::HandleCopyAndDelete()
{
	return false;
}

// construction / destruction 
// (nothing special to do)

CStrictLogIterator::CStrictLogIterator ( const CCachedLogInfo* cachedLog
										 , revision_t startRevision
										 , const CDictionaryBasedPath& startPath)
	: CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CStrictLogIterator::~CStrictLogIterator(void)
{
}

// end namespace LogCache

}
