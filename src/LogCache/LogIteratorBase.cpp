#include "StdAfx.h"
#include "LogIteratorBase.h"

// comparison methods

bool CLogIteratorBase::IntersectsWithPath (const CDictionaryBasedPath& rhsPath) const
{
	return path.IsSameOrParentOf (rhsPath) ||rhsPath.IsSameOrParentOf (path);
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
	, relPath()
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

		HandleCopy();

		// find next entry that mentiones the path
		// stop @ revision 0 or missing log data

		InternalAdvance();
	}
}

void CLogIteratorBase::Retry()
{
	// don't handle copy / rename more than once

	++revision;
	InternalAdvance();
}
