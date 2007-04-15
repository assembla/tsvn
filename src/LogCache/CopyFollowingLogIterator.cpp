#include "StdAfx.h"
#include "CopyFollowingLogIterator.h"

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to -1, if path is deleted.

bool CCopyFollowingLogIterator::HandleCopyAndDelete()
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	size_t index = logInfo->GetRevisions()[revision];

	// switch to new path, if necessary

	return InternalHandleCopyAndDelete ( revisionInfo.GetChangesBegin(index)
									   , revisionInfo.GetChangesEnd(index)
									   , revisionInfo.GetRootPath (index)
									   , path
									   , revision);
}

// construction / destruction 
// (nothing special to do)

CCopyFollowingLogIterator::CCopyFollowingLogIterator 
	( const CCachedLogInfo* cachedLog
	, size_t startRevision
	, const CDictionaryBasedPath& startPath)
	: CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CCopyFollowingLogIterator::~CCopyFollowingLogIterator(void)
{
}

