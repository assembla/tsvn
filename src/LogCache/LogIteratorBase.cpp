#include "StdAfx.h"
#include "LogIteratorBase.h"

// begin namespace LogCache

namespace LogCache
{

// react on cache updates

void CLogIteratorBase::HandleCacheUpdates()
{
	// maybe, we can now use a shorter relative path

	path.RepeatLookup();
}

// comparison methods

bool CLogIteratorBase::PathsIntersect ( const CDictionaryBasedPath& lhsPath
									  , const CDictionaryBasedPath& rhsPath)
{
	return lhsPath.IsSameOrParentOf (rhsPath) 
		|| rhsPath.IsSameOrParentOf (lhsPath);
}

// is current revision actually relevant?

bool CLogIteratorBase::PathInRevision
	( const CRevisionInfoContainer::CChangesIterator& first
	, const CRevisionInfoContainer::CChangesIterator& last
	, const CDictionaryBasedTempPath& path)
{
	// close examination of all changes

	for ( CRevisionInfoContainer::CChangesIterator iter = first
		; iter != last
		; ++iter)
	{
		CDictionaryBasedPath changedPath = iter->GetPath();
		if (changedPath.IsSameOrParentOf (path.GetBasePath()))
			return true;

		// if (and only if) path is a cached path, 
		// it may be a parent of the changedPath

		if (   path.IsFullyCachedPath() 
			&& path.GetBasePath().IsSameOrParentOf (changedPath))
			return true;
	}

	// no paths that we were looking for

	return false;
}

bool CLogIteratorBase::PathInRevision() const
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	revision_t index = logInfo->GetRevisions()[revision];

	// any chance that this revision affects our path?

	CDictionaryBasedPath revisionRootPath = revisionInfo.GetRootPath (index);
	if (!revisionRootPath.IsValid())
		return false;

	if (!PathsIntersect (path.GetBasePath(), revisionRootPath))
		return false;

	// close examination of all changes

	return PathInRevision ( revisionInfo.GetChangesBegin(index)
						  , revisionInfo.GetChangesEnd(index)
						  , path);
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
// Set revision to NO_REVISION, if path is deleted.

bool CLogIteratorBase::InternalHandleCopyAndDelete 
	( const CRevisionInfoContainer::CChangesIterator& first
	, const CRevisionInfoContainer::CChangesIterator& last
	, const CDictionaryBasedPath& revisionRootPath
	, CDictionaryBasedTempPath& searchPath
	, revision_t& searchRevision)
{
	// any chance that this revision affects our search path?

	if (!revisionRootPath.IsValid())
		return false;

	if (!revisionRootPath.IsSameOrParentOf (searchPath.GetBasePath()))
		return false;

	// close examination of all changes

	for ( CRevisionInfoContainer::CChangesIterator iter = first
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
		if (!changedPath.IsSameOrParentOf (searchPath.GetBasePath()))
			continue;

		// now, this is serious

		switch (action)
		{
			// deletion?

			case CRevisionInfoContainer::ACTION_DELETED:
			{
				// end of path history

				searchRevision = NO_REVISION;
				return true;
			}

			// rename?

			case CRevisionInfoContainer::ACTION_ADDED:
			case CRevisionInfoContainer::ACTION_REPLACED:
			{
				// continue search on copy source path

				assert (iter.GetFromPath().IsValid());
				searchPath = searchPath.ReplaceParent ( iter.GetPath()
													  , iter.GetFromPath());
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

// log scanning sub-routines

void CLogIteratorBase::ToNextRevision()
{
	--revision;
}

revision_t CLogIteratorBase::SkipNARevisions()
{
	return logInfo->GetSkippedRevisions()
		.GetPreviousRevision (path.GetBasePath(), revision);
}

// log scanning

void CLogIteratorBase::InternalAdvance()
{
	// find next entry that mentiones the path
	// stop @ revision 0 or missing log data

	do
	{
		// perform at least one step

		ToNextRevision();

		// skip ranges of missing data, if we know
		// that they don't affect our path

		while (InternalDataIsMissing())
		{
			revision_t nextRevision = SkipNARevisions(); 
			if (nextRevision != NO_REVISION)
				revision = nextRevision;
		}
	}
	while ((revision > 0) && !InternalDataIsMissing() && !PathInRevision());
}

// construction 
// (copy construction & assignment use default methods)

CLogIteratorBase::CLogIteratorBase ( const CCachedLogInfo* cachedLog
								   , revision_t startRevision
								   , const CDictionaryBasedTempPath& startPath)
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
	// maybe, there was some cache update

	HandleCacheUpdates();

	// end of history?

	if (revision > 0)
	{
		// the current revision may be a copy / rename
		// -> update our path before we proceed, if necessary

		if (HandleCopyAndDelete())
		{
			// revision may have been set to NO_REVISION, 
			// e.g. if a deletion has been found

			if (revision != NO_REVISION)
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
	// maybe, there was some cache update

	HandleCacheUpdates();

	// don't handle copy / rename more than once

	++revision;
	InternalAdvance();
}

// end namespace LogCache

}

