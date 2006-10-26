#include "StdAfx.h"
#include ".\skiprevisioninfo.h"

#include ".\RevisionIndex.h"
#include ".\PathDictionary.h"

// find next / previous "gap"

DWORD CSkipRevisionInfo::SPerPathRanges::FindNext (DWORD revision) const
{
	// special case

	if (ranges.empty())
		return -1;

	// look for the first range *behind* revision

	TRanges::const_iterator iter = ranges.upper_bound (revision);
	if (iter == ranges.begin())
		return -1;

	// return end of previous range, if revision is within this range

	--iter;
	DWORD next = iter->first + iter->second;
	return next <= revision 
		? -1
		: next;
}

DWORD CSkipRevisionInfo::SPerPathRanges::FindPrevious (DWORD revision) const
{
	// special case

	if (ranges.empty())
		return -1;

	// look for the first range *behind* revision

	TRanges::const_iterator iter = ranges.upper_bound (revision);
	if (iter == ranges.begin())
		return -1;

	// return start-1 of previous range, if revision is within this range

	--iter;
	DWORD next = iter->first + iter->second;
	return next <= revision 
		? -1
		: iter->first-1;
}

// update / insert range

void CSkipRevisionInfo::SPerPathRanges::Add (DWORD start, DWORD size)
{
	DWORD end = start + size;

	// insert the new range / enlarge existing range

	TRanges::_Pairib insertionResult = ranges.insert (std::make_pair (start, size));
	if (!insertionResult.second && (insertionResult.first->second < size))
		insertionResult.first->second = size;

	// merge it with the previous one

	if (insertionResult.first != ranges.begin())
	{
		TRanges::iterator iter = insertionResult.first;
		--iter;

		DWORD previousEnd = iter->first + iter->second;

		if (previousEnd >= start)
		{
			if (end < previousEnd)
				end = previousEnd;

			iter->second = end - iter->first;

			insertionResult.first = ranges.erase (insertionResult.first);
			--insertionResult.first;
		}
	}

	// merge it with the next one

	TRanges::iterator iter = insertionResult.first;
	if ((++iter != ranges.end()) && (iter->first <= end))
	{
		insertionResult.first->second = max (end, iter->first + iter->second) 
									  - insertionResult.first->first;
		ranges.erase (iter);
	}
}

// remove known revisions from the range

void CSkipRevisionInfo::TryReduceRange (DWORD& revision, DWORD& size)
{
	// raise lower bound

	while ((size > 0) && ((*revisions)[revision] != -1))
	{
		++revision;
		--size;
	}

	// lower upper bound

	while ((size > 0) && ((*revisions)[revision + size-1] != -1))
	{
		--size;
	}
}

// construction / destruction

CSkipRevisionInfo::CSkipRevisionInfo ( const CPathDictionary* aPathDictionary
									 , const CRevisionIndex* aRevisionIndex)
	: index (CHashFunction (&data))
	, paths (aPathDictionary)
	, revisions (aRevisionIndex)
{
}

CSkipRevisionInfo::~CSkipRevisionInfo(void)
{
	Clear();
}

// query data

DWORD CSkipRevisionInfo::GetNextRevision ( const CDictionaryBasedPath& path
										 , DWORD revision) const
{
	// above the root or invalid parameter ?

	if (!path.IsValid() || (revision == -1))
		return -1;

	// lookup the extry for this path

	DWORD dataIndex = index.find ((DWORD)path.GetIndex());
	SPerPathRanges* ranges = dataIndex == -1
						   ? NULL
						   : data[dataIndex];

	// crawl this and the parent path data
	// until we found a gap (i.e. could not improve further)

	DWORD startRevision = revision;
	DWORD result = revision;

	do
	{
		result = revision;

		// for 

		DWORD parentNext = GetNextRevision (path.GetParent(), revision);
		if (parentNext != -1)
			revision = parentNext;

		if (ranges != NULL)
		{
			DWORD next = ranges->FindNext (revision);
			if (parentNext != -1)
				revision = parentNext;
		}
	}
	while (revision > result);

	// ready

	return revision == startRevision 
		? -1 
		: result;
}

DWORD CSkipRevisionInfo::GetPreviousRevision ( const CDictionaryBasedPath& path
											 , DWORD revision) const
{
	// above the root or invalid parameter ?

	if (!path.IsValid() || (revision == -1))
		return -1;

	// lookup the extry for this path

	DWORD dataIndex = index.find ((DWORD)path.GetIndex());
	SPerPathRanges* ranges = dataIndex == -1
						   ? NULL
						   : data[dataIndex];

	// crawl this and the parent path data
	// until we found a gap (i.e. could not improve further)

	DWORD startRevision = revision;
	DWORD result = revision;

	do
	{
		result = revision;

		// for 

		DWORD parentNext = GetPreviousRevision (path.GetParent(), revision);
		if (parentNext != -1)
			revision = parentNext;

		if (ranges != NULL)
		{
			DWORD next = ranges->FindPrevious (revision);
			if (parentNext != -1)
				revision = parentNext;
		}
	}
	while (revision < result);

	// ready

	return revision == startRevision 
		? -1 
		: result;
}

// add / remove data

void CSkipRevisionInfo::Add ( const CDictionaryBasedPath& path
							, DWORD revision
							, DWORD size)
{
	// violating these assertions will break our lookup algorithms

	assert (path.IsValid());
	assert (revision > 0);
	assert (revision != -1);
	assert (size != -1);

	// reduce the range, if we have revision info for the boundaries

	TryReduceRange (revision, size);
	if (size == 0)
		return;

	// lookup / auto-insert entry for path

	SPerPathRanges* ranges = NULL;
	DWORD dataIndex = index.find ((DWORD)path.GetIndex());

	if (dataIndex == -1)
	{
		ranges = new SPerPathRanges;
		data.push_back (ranges);
		index.insert ((DWORD)path.GetIndex(), (DWORD)data.size()-1);
	}
	else
	{
		ranges = data[dataIndex];
	}

	// add range

	ranges->Add (revision, size);
}

void CSkipRevisionInfo::Clear()
{
	for (size_t i = 0, count = data.size(); i != count; ++i)
		delete data[i];

	data.clear();
	index.clear();
}

// remove unnecessary entries

void CSkipRevisionInfo::Compress()
{
	std::vector<IT> allRanges;

	// remove all parent ranges

	for (size_t i = 0, count = data.size(); i < count; ++i)
	{
		CDictionaryBasedPath parentPath 
			= CDictionaryBasedPath (paths, data[i]->pathID).GetParent();

		SPerPathRanges::TRanges& ranges = data[i]->ranges;
		SPerPathRanges::TRanges::iterator iter = ranges.begin();

		while (iter != ranges.end())
		{
			bool removed = false;
			DWORD next = GetNextRevision (parentPath, iter->first);
			if (next != -1)
			{
				if (next >= iter->first + iter->second)
				{
					iter = ranges.erase (iter);
					removed = true;
				}
				else
				{
					DWORD size = iter->second + iter->first - next;
					iter = ranges.erase (iter);
					iter = ranges.insert (iter, std::make_pair (next, size));
				}
			}

			if (!removed)
			{
				DWORD end = iter->first + iter->second-1;
				DWORD previous = GetPreviousRevision (parentPath, end);
				if (previous != -1)
				{
					if (previous < iter->first)
					{
						iter = ranges.erase (iter);
						removed = true;
					}
					else
					{
						iter->second = previous - iter->first + 1;
					}
				}
			}

			if (!removed)
			{
				allRanges.push_back (iter);
				++iter;
			}
		}
	}

	// sort all ranges

	std::sort (allRanges.begin(), allRanges.end(), CIterComp());

	// reset all ranges that are completely covered by cached revision info

	DWORD firstKnownRevision = 0;
	DWORD nextUnknownRevision = 0;
	DWORD lastRevision = (DWORD)revisions->GetLastRevision();

	for (size_t i = 0; i < allRanges.size(); ++i)
	{
		IT& iter = allRanges[i];
		if (iter->first > nextUnknownRevision)
		{
			firstKnownRevision = iter->first;
			while (((*revisions)[firstKnownRevision] == -1) && (firstKnownRevision < lastRevision))
				++firstKnownRevision;

			nextUnknownRevision = firstKnownRevision+1;
			while (((*revisions)[nextUnknownRevision] != -1) && (nextUnknownRevision < lastRevision))
				++nextUnknownRevision;
		}

		if (iter->first + iter->second <= nextUnknownRevision)
			iter->second = 0;
	}

	// remove all ranges that have been reset to size 0

	std::vector<SPerPathRanges*>::iterator dest = data.begin();
	for (size_t i = 0, count = data.size(); i < count; ++i)
	{
		SPerPathRanges::TRanges& ranges = data[i]->ranges;
		for (IT iter = ranges.begin(); iter != ranges.end(); )
		{
			if (iter->second == 0)
				iter = ranges.erase (iter);
			else
				++iter;
		}

		if (ranges.empty())
		{
			delete data[i];
		}
		else
		{
			*dest = data[i];
			++dest;
		}
	}

	data.erase (dest, data.end());
}
