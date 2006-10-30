#include "StdAfx.h"
#include ".\skiprevisioninfo.h"

#include ".\RevisionIndex.h"
#include ".\PathDictionary.h"

#include ".\PackedDWORDInStream.h"
#include ".\PackedDWORDOutStream.h"
#include ".\DiffIntegerInStream.h"
#include ".\DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo::SPerPathRanges
///////////////////////////////////////////////////////////////
// find next / previous "gap"
///////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////
// update / insert range
///////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo::CPacker
///////////////////////////////////////////////////////////////
// remove ranges already covered by parent path ranges
///////////////////////////////////////////////////////////////

size_t CSkipRevisionInfo::CPacker::RemoveParentRanges()
{
	// count the number of remaining ranges

	size_t rangeCount = 0;

	// remove all parent ranges

	std::vector<SPerPathRanges*>& data = parent->data;
	const CPathDictionary& paths = parent->paths;

	for (size_t i = 0, count = parent->data.size(); i < count; ++i)
	{
		SPerPathRanges* perPathInfo = parent->data[i];
		CDictionaryBasedPath parentPath 
			= CDictionaryBasedPath (&paths, perPathInfo->pathID).GetParent();

		SPerPathRanges::TRanges& ranges = perPathInfo->ranges;
		IT iter = ranges.begin();

		// check all ranges of data[i]

		while (iter != ranges.end())
		{
			bool removed = false;
			DWORD next = parent->GetNextRevision (parentPath, iter->first);

			// does the parent cover at least the begin of this range?

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
					ranges.insert (iter, std::make_pair (next, size));
					iter = ranges.erase (iter);
				}
			}

			if (!removed)
			{
				// the range wasn't entierely covered

				DWORD end = iter->first + iter->second-1;
				DWORD previous = parent->GetPreviousRevision (parentPath, end);

				// parent covers the end of the range?

				if (previous != -1)
				{
					// must be no complete cover

					assert (previous >= iter->first);

					// reduce the size of the range

					iter->second = previous - iter->first + 1;
				}
			}

			// update counter and iterator

			if (!removed)
			{
				++rangeCount;
				++iter;
			}
		}
	}

	// sort all ranges

	return rangeCount;
}

///////////////////////////////////////////////////////////////
// build a sorted list of all ranges
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::SortRanges (size_t rangeCount)
{
	allRanges.clear();
	allRanges.reserve (rangeCount);

	// remove all parent ranges

	std::vector<SPerPathRanges*>& data = parent->data;
	for (size_t i = 0, count = data.size(); i < count; ++i)
	{
		SPerPathRanges::TRanges& ranges = data[i]->ranges;
		for (IT iter = ranges.begin(), end = ranges.end(); iter != end; ++iter)
		{
			allRanges.push_back (iter);
		}
	}

	// sort all ranges

	std::sort (allRanges.begin(), allRanges.end(), CIterComp());
}

///////////////////////////////////////////////////////////////
// reset ranges completely covered by cached revision info
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::RemoveKnownRevisions()
{
	const CRevisionIndex& revisions = parent->revisions;

	DWORD firstKnownRevision = 0;
	DWORD nextUnknownRevision = 0;
	DWORD lastRevision = (DWORD)revisions.GetLastRevision();

	for (size_t i = 0; i < allRanges.size(); ++i)
	{
		IT& iter = allRanges[i];
		if (iter->first > nextUnknownRevision)
		{
			firstKnownRevision = iter->first;
			while ((revisions[firstKnownRevision] == -1) && (firstKnownRevision < lastRevision))
				++firstKnownRevision;

			nextUnknownRevision = firstKnownRevision+1;
			while ((revisions[nextUnknownRevision] != -1) && (nextUnknownRevision < lastRevision))
				++nextUnknownRevision;
		}

		if (iter->first + iter->second <= nextUnknownRevision)
			iter->second = 0;
	}
}

///////////////////////////////////////////////////////////////
// remove all ranges that have been reset to size 0
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::RemoveEmptyRanges()
{
	// remove all ranges that have been reset to size 0

	std::vector<SPerPathRanges*>& data = parent->data;

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

		// remove path data if there are no ranges left 
		// for the respecitive path ID

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

///////////////////////////////////////////////////////////////
// construction / destruction
///////////////////////////////////////////////////////////////

CSkipRevisionInfo::CPacker::CPacker()
	: parent (NULL)
{
}

CSkipRevisionInfo::CPacker::~CPacker()
{
}

///////////////////////////////////////////////////////////////
// remove unnecessary entries
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::operator()(CSkipRevisionInfo* aParent)
{
	parent = aParent;

	size_t rangeCount = RemoveParentRanges();
	SortRanges (rangeCount);
	RemoveKnownRevisions();
	RemoveEmptyRanges();
}

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo
///////////////////////////////////////////////////////////////
// remove known revisions from the range
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::TryReduceRange (DWORD& revision, DWORD& size)
{
	// raise lower bound

	while ((size > 0) && (revisions[revision] != -1))
	{
		++revision;
		--size;
	}

	// lower upper bound

	while ((size > 0) && (revisions[revision + size-1] != -1))
	{
		--size;
	}
}

///////////////////////////////////////////////////////////////
// construction / destruction
///////////////////////////////////////////////////////////////

CSkipRevisionInfo::CSkipRevisionInfo ( const CPathDictionary& aPathDictionary
									 , const CRevisionIndex& aRevisionIndex)
	: index (CHashFunction (&data))
	, paths (aPathDictionary)
	, revisions (aRevisionIndex)
{
}

CSkipRevisionInfo::~CSkipRevisionInfo(void)
{
	Clear();
}

///////////////////////////////////////////////////////////////
// query data
///////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////
// add / remove data
///////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////
// remove all data
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::Clear()
{
	for (size_t i = 0, count = data.size(); i != count; ++i)
		delete data[i];

	data.clear();
	index.clear();
}

///////////////////////////////////////////////////////////////
// remove unnecessary entries
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::Compress()
{
	CPacker()(this);
}

///////////////////////////////////////////////////////////////
// stream I/O
///////////////////////////////////////////////////////////////

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CSkipRevisionInfo& container)
{
	// open sub-streams

	CPackedDWORDInStream* pathIDsStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::PATHIDS_STREAM_ID));

	CPackedDWORDInStream* entryCountStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::ENTRY_COUNT_STREAM_ID));

	CDiffDWORDInStream* revisionsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::REVISIONS_STREAM_ID));

	CDiffDWORDInStream* sizesStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::SIZES_STREAM_ID));

	// read all data

	size_t count = pathIDsStream->GetValue();

	container.Clear();
	container.data.reserve (count);

	for (size_t i = 0; i < count; ++i)
	{
		std::auto_ptr<CSkipRevisionInfo::SPerPathRanges> perPathInfo 
			(new CSkipRevisionInfo::SPerPathRanges);

		perPathInfo->pathID = pathIDsStream->GetValue();

		size_t entryCount = entryCountStream->GetValue();
		CSkipRevisionInfo::IT iter = perPathInfo->ranges.end();
		for (size_t k = 0; i < entryCount; ++k)
		{
			iter = perPathInfo->ranges.insert 
					(iter, std::make_pair ( revisionsStream->GetValue()
										  , sizesStream->GetValue()));
		}

		container.data.push_back (perPathInfo.release());
	}

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CSkipRevisionInfo& container)
{
	// open sub-streams

	CPackedDWORDOutStream* pathIDsStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::PATHIDS_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));

	CPackedDWORDOutStream* entryCountStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::ENTRY_COUNT_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));

	CDiffDWORDOutStream* revisionsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::REVISIONS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));

	CDiffDWORDOutStream* sizesStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::SIZES_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));

	// write all data

	pathIDsStream->Add ((DWORD)container.data.size());

	typedef std::vector<CSkipRevisionInfo::SPerPathRanges*>::const_iterator CIT;
	for ( CIT dataIter = container.data.begin(), dataEnd = container.data.end()
		; dataIter < dataEnd
		; ++dataIter)
	{
		pathIDsStream->Add ((*dataIter)->pathID);
		entryCountStream->Add ((DWORD)(*dataIter)->ranges.size());

		for ( CSkipRevisionInfo::IT iter = (*dataIter)->ranges.begin()
			, end = (*dataIter)->ranges.end()
			; iter != end
			; ++iter)
		{
			revisionsStream->Add (iter->first);
			sizesStream->Add (iter->second);
		}
	}

	// ready

	return stream;
}

