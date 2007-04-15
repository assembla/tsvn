#include "StdAfx.h"
#include "LogBatchIteratorBase.h"

// find the top revision of all paths

size_t CLogBatchIteratorBase::MaxRevision 
	(const TPathRevisions& pathRevisions)
{
	if (pathRevisions.empty())
		return -1;
	
	size_t result = pathRevisions[0].second;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin() + 1
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		if (result < iter->second)
			result = iter->second;
	}

	return result;
}

// find the top revision of all paths

CDictionaryBasedPath CLogBatchIteratorBase::BasePath 
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& pathRevisions)
{
	// return a dummy path, if there is nothing to log

	if (pathRevisions.empty())
		return CDictionaryBasedPath ( &cachedLog->GetLogInfo().GetPaths()
									, (size_t)-1);
	
	// fold the paths

	CDictionaryBasedPath result = pathRevisions[0].first;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin() + 1
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		result = result.GetCommonRoot (iter->first);
	}

	// ready

	return result;
}

// construction 
// (copy construction & assignment use default methods)

CLogBatchIteratorBase::CLogBatchIteratorBase 
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& startPathRevisions)
	: CLogIteratorBase ( cachedLog
					   , MaxRevision (startPathRevisions)
					   , BasePath (cachedLog, startPathRevisions))
	, pathRevisions (startPathRevisions)
{
}

// navigation sub-routines

size_t CLogBatchIteratorBase::SkipNARevisions()
{
	const CSkipRevisionInfo& skippedRevisions = logInfo->GetSkippedRevisions();

	size_t result = -1;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		// test at least the start revision of any path

		size_t localResult = min (revision, iter->second);

		// still relevant?

		if ((result == -1) || (localResult > result))
		{
			// skip revisions that are of no interest for this path

			localResult
				= skippedRevisions.GetPreviousRevision ( iter->first
													   , localResult);

			// keep the max. of all next relevant revisions for

			if ((result == -1) || (localResult > result))
				result = localResult;
		}
	}

	// ready

	return result;
}

bool CLogBatchIteratorBase::PathInRevision() const
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	size_t index = logInfo->GetRevisions()[revision];

	// fetch invariant information

	CDictionaryBasedPath revisionRootPath 
		= revisionInfo.GetRootPath (index);

	CRevisionInfoContainer::CChangesIterator first 
		= revisionInfo.GetChangesBegin (index);
	CRevisionInfoContainer::CChangesIterator last 
		= revisionInfo.GetChangesEnd (index);

	// test all relevant paths

	for ( TPathRevisions::const_iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		// is path relevant?

		if (   (iter->second >= revision)
			&& !PathsIntersect (iter->first, revisionRootPath))
		{
			// if we found one match, we are done

			if (CLogIteratorBase::PathInRevision (first, last, iter->first))
				return true;
		}
	}

	// no matching entry found

	return false;
}

// log scanning: scan on multiple paths

void CLogBatchIteratorBase::ToNextRevision()
{
	// find the next entry that might be relevant
	// for at least one of our paths

	CLogIteratorBase::InternalAdvance();
}

// destruction

CLogBatchIteratorBase::~CLogBatchIteratorBase(void)
{
}
