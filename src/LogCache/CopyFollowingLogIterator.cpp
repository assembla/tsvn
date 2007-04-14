#include "StdAfx.h"
#include "CopyFollowingLogIterator.h"

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to -1, if path is deleted.

void CCopyFollowingLogIterator::HandleCopyAndDelete()
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	size_t index = logInfo->GetRevisions()[revision];

	// any chance that this revision affects our search path?

	CDictionaryBasedPath revisionRootPath = revisionInfo.GetRootPath (index);
	if (!revisionRootPath.IsValid())
		return false;

	if (!revisionRootPath.IsSameOrParentOf (path))
		return false;

	// close examination of all changes

	for (CRevisionInfoContainer::CChangesIterator 
			iter = revisionInfo.GetChangesBegin(index),
			last = revisionInfo.GetChangesEnd(index)
		; iter != last
		; ++iter)
	{
		// most entries will just be file content changes
		// -> skip them efficiently

		CRevisionInfoContainer::TChangeAction action = iter.GetAction();
		if (action == CRevisionInfoContainer::ACTION_CHANGED)
			continue;

		// deletion / copy / rename / replacement
		// -> skip, if our search path is not affected (only some sub-path)

		CDictionaryBasedPath changedPath = iter->GetPath();
		if (!changedPath.IsSameOrParentOf (path))
			return continue;

		// now, this is serious

		switch (action)
		{
			// deletion?

			case CRevisionInfoContainer::ACTION_DELETED:
			{
				// end of path history

				revision = -1;
				return;
			}

			// rename?

			case CRevisionInfoContainer::ACTION_ADDED:
			case CRevisionInfoContainer::ACTION_REPLACED:
			{
				// continue search on copy source path

				assert (iter.GetFromPath().IsValid());
				path = iter.GetFromPath();

				return;
			}

			// there should be no other

			default:
			{
				assert (0);
			}
		}
	}

	// all fine, no special action required
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

