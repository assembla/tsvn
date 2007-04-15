#include "StdAfx.h"
#include "LogIteratorBase.h"

// comparison methods

bool CLogIteratorBase::IntersectsWithPath (const CDictionaryBasedPath& rhsPath) const
{
	return path.IsSameOrParentOf (rhsPath) || rhsPath.IsSameOrParentOf (path);
}

bool CLogIteratorBase::PathInRevision() const
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	size_t index = logInfo->GetRevisions()[revision];

	// any chance that this revision affects our path?

	CDictionaryBasedPath revisionRootPath = revisionInfo.GetRootPath (index);
	if (!revisionRootPath.IsValid())
		return false;

	if (!IntersectsWithPath (revisionRootPath))
		return false;

	// close examination of all changes

	for (CRevisionInfoContainer::CChangesIterator 
			iter = revisionInfo.GetChangesBegin(index),
			last = revisionInfo.GetChangesEnd(index)
		; iter != last
		; ++iter)
	{
		CDictionaryBasedPath changedPath = iter->GetPath();
		if (IntersectsWithPath (revisionRootPath))
			return true;
	}

	// no paths that we were looking for

	return false;
}

// Test, whether InternalHandleCopyAndDelete() should be used

bool CLogIteratorBase::ContainsCopyOrDelete 
	( const CRevisionInfoContainer::CChangesIterator& first
	, const CRevisionInfoContainer::CChangesIterator& last)
{
	// close examination of all changes

	for (CRevisionInfoContainer::CChangesIterator iter = first
		; iter != last
		; ++iter)
	{
		// the only non-critical operation is the mere modification

		CRevisionInfoContainer::TChangeAction action = iter.GetAction();
		if (action != CRevisionInfoContainer::ACTION_CHANGED)
			return true;
	}

	// no copy / delete / replace found

	return false;
}

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to -1, if path is deleted.

bool CLogIteratorBase::InternalHandleCopyAndDelete 
	( const CRevisionInfoContainer::CChangesIterator& first
	, const CRevisionInfoContainer::CChangesIterator& last
	, const CDictionaryBasedPath& revisionRootPath
	, CDictionaryBasedPath& searchPath
	, size_t& searchRevision)
{
	// any chance that this revision affects our search path?

	if (!revisionRootPath.IsValid())
		return false;

	if (!revisionRootPath.IsSameOrParentOf (searchPath))
		return false;

	// close examination of all changes

	for (CRevisionInfoContainer::CChangesIterator iter = first
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
		if (!changedPath.IsSameOrParentOf (searchPath))
			continue;

		// now, this is serious

		switch (action)
		{
			// deletion?

			case CRevisionInfoContainer::ACTION_DELETED:
			{
				// end of path history

				searchRevision = -1;
				return true;
			}

			// rename?

			case CRevisionInfoContainer::ACTION_ADDED:
			case CRevisionInfoContainer::ACTION_REPLACED:
			{
				// continue search on copy source path

				assert (iter.GetFromPath().IsValid());
				searchPath = iter.GetFromPath();
				searchRevision = iter.GetFromRevision();

				return true;
			}

			// there should be no other

			default:
			{
				assert (0);
			}
		}
	}

	// all fine, no special action required

	return false;
}

// log scanning

void CLogIteratorBase::InternalAdvance()
{
	// find next entry that mentiones the path
	// stop @ revision 0 or missing log data

	do
	{
		// perform at least one step

		--revision;

		// skip ranges of missing data, if we know
		// that they don't affect our path

		while (InternalDataIsMissing())
		{
			DWORD nextRevision 
				= logInfo->GetSkippedRevisions()
					.GetPreviousRevision (path, revision);

			if (nextRevision != -1)
				revision = nextRevision;
		}
	}
	while ((revision > 0) && !InternalDataIsMissing() && !PathInRevision());
}

// construction 
// (copy construction & assignment use default methods)

CLogIteratorBase::CLogIteratorBase ( const CCachedLogInfo* cachedLog
								   , size_t startRevision
								   , const CDictionaryBasedPath& startPath)
	: logInfo (cachedLog)
	, revision (startRevision)
	, path (startPath)
{
}

CLogIteratorBase::~CLogIteratorBase(void)
{
}

// implement ILogIterator

bool CLogIteratorBase::DataIsMissing() const
{
	return InternalDataIsMissing();
}

void CLogIteratorBase::Advance()
{
	if (revision > 0)
	{
		// the current revision may be a copy / rename
		// -> update our path before we proceed, if necessary

		if (HandleCopyAndDelete())
		{
			// revision may have been set to -1, 
			// e.g. if a deletion has been found

			if (revision != -1)
			{
				// switched to a new path
				// -> retry access on that new path 
				// (especially, we must try (copy-from-) revision)

				Retry();
			}
		}
		else
		{
			// find next entry that mentiones the path
			// stop @ revision 0 or missing log data

			InternalAdvance();
		}
	}
}

void CLogIteratorBase::Retry()
{
	// don't handle copy / rename more than once

	++revision;
	InternalAdvance();
}
