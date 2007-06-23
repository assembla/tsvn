// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include ".\revisioninfocontainer.h"

#include "PackedDWORDInStream.h"
#include "PackedDWORDOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"
#include "PackedTime64InStream.h"
#include "PackedTime64OutStream.h"

// begin namespace LogCache

namespace LogCache
{

// update / modify utilities

void CRevisionInfoContainer::UpdateAuthors 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap)
{
	index_mapping_t idMapping = authorPool.Merge (newData.authorPool);

	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		if (iter->key < count)
			authors[iter->key] = *idMapping.find (newData.authors[iter->value]);
	}
}

void CRevisionInfoContainer::UpdateTimeStamps 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap)
{
	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		if (iter->key < count)
			timeStamps[iter->key] = newData.timeStamps[iter->value];
	}
}

void CRevisionInfoContainer::UpdateComments 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap)
{
	index_mapping_t toReplace;

	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		if (iter->key < count)
			toReplace.insert (iter->key, iter->value);
	}

	comments.Replace (newData.comments, toReplace);
}

void CRevisionInfoContainer::UpdateChanges 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
	, const index_mapping_t& pathIDMapping)
{
	// save & remove old data

	std::vector<unsigned char> oldChanges;
	changes.swap (oldChanges);
	changes.reserve (oldChanges.size());

	std::vector<index_t> oldChangedPaths;
	changedPaths.swap (oldChangedPaths);
	changedPaths.reserve (oldChangedPaths.size());

	std::vector<index_t> oldCopyFromPaths;
	copyFromPaths.swap (oldCopyFromPaths);
	copyFromPaths.reserve (oldCopyFromPaths.size());

	std::vector<revision_t> oldCopyFromRevisions;
	copyFromRevisions.swap (oldCopyFromRevisions);
	copyFromRevisions.reserve (oldCopyFromRevisions.size());

	// splice

	index_t firstChange = changesOffsets[0];
	index_t firstCopy = copyFromOffsets[0];

	index_mapping_t::const_iterator mapEnd = indexMap.end();
	for (index_t i = 0, count = size(); i < count; ++i)
	{
		index_mapping_t::const_iterator iter = indexMap.find (i);
		if (iter != mapEnd)
		{
			// copy & translate

			index_t sourceIndex = iter->value;
			for ( index_t k = newData.changesOffsets [sourceIndex]
				, last = newData.changesOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				changes.push_back (newData.changes[k]);
				changedPaths.push_back (*pathIDMapping.find (newData.changedPaths[k]));
			}

			for ( index_t k = newData.copyFromOffsets [sourceIndex]
				, last = newData.copyFromOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				copyFromRevisions.push_back (newData.copyFromRevisions[k]);
				copyFromPaths.push_back (*pathIDMapping.find (newData.copyFromPaths[k]));
			}
		}
		else
		{
			// keep exisiting data

			// standard per-path info

			index_t lastChange = changesOffsets[i+1];

			changes.insert ( changes.end()
						   , oldChanges.begin() + firstChange
						   , oldChanges.begin() + lastChange);
			changedPaths.insert ( changedPaths.end()
								, oldChangedPaths.begin() + firstChange
								, oldChangedPaths.begin() + lastChange);

			// copy-from info, if available

			index_t lastCopy = copyFromOffsets[i+1];

			if (firstCopy != lastCopy)
			{
				copyFromPaths.insert ( copyFromPaths.end()
								     , oldCopyFromPaths.begin() + firstCopy
								     , oldCopyFromPaths.begin() + lastCopy);
				copyFromRevisions.insert ( copyFromRevisions.end()
										 , oldCopyFromRevisions.begin() + firstCopy
										 , oldCopyFromRevisions.begin() + lastCopy);
			}
		}

		// update positions

		firstChange = changesOffsets[i+1];
		firstCopy = copyFromOffsets[i+1];

		changesOffsets[i+1] = static_cast<index_t>(changes.size());
		copyFromOffsets[i+1] = static_cast<index_t>(copyFromPaths.size());
	}
}

void CRevisionInfoContainer::UpdateMergers 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
	, const index_mapping_t& pathIDMapping)
{
	// save & remove old data

	std::vector<index_t> oldMergedFromPaths;
	mergedFromPaths.swap (oldMergedFromPaths);
	mergedFromPaths.reserve (oldMergedFromPaths.size());

	std::vector<index_t> oldMergedToPaths;
	mergedToPaths.swap (oldMergedToPaths);
	mergedToPaths.reserve (oldMergedToPaths.size());

	std::vector<revision_t> oldMergedRangeStarts;
	mergedRangeStarts.swap (oldMergedRangeStarts);
	mergedRangeStarts.reserve (oldMergedRangeStarts.size());

	std::vector<revision_t> oldMergedRangeDeltas;
	mergedRangeDeltas.swap (oldMergedRangeDeltas);
	mergedRangeDeltas.reserve (oldMergedRangeDeltas.size());

	// splice

	index_t firstMerge = mergedRevisionsOffsets[0];

	index_mapping_t::const_iterator mapEnd = indexMap.end();
	for (index_t i = 0, count = size(); i < count; ++i)
	{
		index_mapping_t::const_iterator iter = indexMap.find (i);
		if (iter != mapEnd)
		{
			// copy & translate

			index_t sourceIndex = iter->value;
			for ( index_t k = newData.mergedRevisionsOffsets [sourceIndex]
				, last = newData.mergedRevisionsOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				mergedFromPaths.push_back (*pathIDMapping.find (newData.mergedFromPaths[k]));
				mergedToPaths.push_back (*pathIDMapping.find (newData.mergedToPaths[k]));

				mergedRangeStarts.push_back (newData.mergedRangeStarts[k]);
				mergedRangeDeltas.push_back (newData.mergedRangeDeltas[k]);
			}
		}
		else
		{
			// keep exisiting data

			index_t lastMerge = mergedRevisionsOffsets[i+1];

			mergedFromPaths.insert ( mergedFromPaths.end()
								   , oldMergedFromPaths.begin() + firstMerge
								   , oldMergedFromPaths.begin() + lastMerge);
			mergedToPaths.insert ( mergedToPaths.end()
								 , oldMergedToPaths.begin() + firstMerge
								 , oldMergedToPaths.begin() + lastMerge);
			mergedRangeStarts.insert ( mergedRangeStarts.end()
									 , oldMergedRangeStarts.begin() + firstMerge
									 , oldMergedRangeStarts.begin() + lastMerge);
			mergedRangeDeltas.insert ( mergedRangeDeltas.end()
									 , oldMergedRangeDeltas.begin() + firstMerge
									 , oldMergedRangeDeltas.begin() + lastMerge);
		}

		// update positions

		firstMerge = mergedRevisionsOffsets[i+1];
		mergedRevisionsOffsets[i+1] = static_cast<index_t>(mergedFromPaths.size());
	}
}

void CRevisionInfoContainer::Append 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
	, const index_mapping_t& pathIDMapping)
{
	// append new entries

	size_t oldCount = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		if (iter->key < oldCount)
			continue;

		index_t i = iter->value;

		// copy & translate change info

		for ( index_t k = newData.changesOffsets[i]
			, last = newData.changesOffsets[i+1]
			; k != last
			; ++k)
		{
			changedPaths.push_back (*pathIDMapping.find (newData.changedPaths[k]));
			changes.push_back (newData.changes[k]);
		}

		changesOffsets.push_back ((index_t)changes.size());

		// copy & translate copy-from info

		for ( index_t k = newData.copyFromOffsets[i]
			, last = newData.copyFromOffsets[i+1]
			; k != last
			; ++k)
		{
			copyFromPaths.push_back (*pathIDMapping.find (newData.copyFromPaths[k]));
			copyFromRevisions.push_back (newData.copyFromRevisions[k]);
		}

		copyFromOffsets.push_back ((index_t)copyFromPaths.size());

		// copy & translate merge info

		for ( index_t k = newData.mergedRevisionsOffsets[i]
			, last = newData.mergedRevisionsOffsets[i+1]
			; k != last
			; ++k)
		{
			mergedFromPaths.push_back (*pathIDMapping.find (newData.mergedFromPaths[k]));
			mergedToPaths.push_back (*pathIDMapping.find (newData.mergedToPaths[k]));

			mergedRangeStarts.push_back (newData.mergedRangeStarts[k]);
			mergedRangeDeltas.push_back (newData.mergedRangeDeltas[k]);
		}

		mergedRevisionsOffsets.push_back ((index_t)mergedFromPaths.size());
	}
}

// construction / destruction

CRevisionInfoContainer::CRevisionInfoContainer(void)
{
	// [lastIndex+1] must point to [size()]

	changesOffsets.push_back(0);
	copyFromOffsets.push_back(0);
	mergedRevisionsOffsets.push_back(0);
}

CRevisionInfoContainer::~CRevisionInfoContainer(void)
{
}

// add information
// AddChange() always adds to the last revision

index_t CRevisionInfoContainer::Insert ( const std::string& author
									   , const std::string& comment
									   , __time64_t timeStamp)
{
	// this should newer throw as there are usually more
	// changes than revisions. But you never know ...

	if (authors.size() == NO_INDEX)
		throw std::exception ("revision container overflow");

	// store the given revision info

	authors.push_back (authorPool.AutoInsert (author.c_str()));
	timeStamps.push_back (timeStamp);
	comments.Insert (comment);

	// no changes yet -> no common root path info

	rootPaths.push_back (NO_INDEX);

	// empty range for changes 

	changesOffsets.push_back (*changesOffsets.rbegin());
	copyFromOffsets.push_back (*copyFromOffsets.rbegin());
	mergedRevisionsOffsets.push_back (*mergedRevisionsOffsets.rbegin());

	// ready

	return size()-1;
}

void CRevisionInfoContainer::AddChange ( TChangeAction action
									   , const std::string& path
									   , const std::string& fromPath
									   , revision_t fromRevision)
{
	// under x64, there might actually be an overflow

	if (changes.size() == NO_INDEX)
		throw std::exception ("revision container change list overflow");

	// another change

	++(*changesOffsets.rbegin());

	// add change path index and update the root path 
	// of all changes in this revision

	CDictionaryBasedPath parsedPath (&paths, path, false);
	changedPaths.push_back (parsedPath.GetIndex());

	index_t& rootPathIndex = *rootPaths.rbegin();
	rootPathIndex = rootPathIndex == NO_INDEX
				  ? parsedPath.GetIndex()
				  : parsedPath.GetCommonRoot (rootPathIndex).GetIndex();

	// add changes info (flags), and indicate presence of fromPath (if given)

	if (fromPath.empty())
	{
		changes.push_back ((char)action);
	}
	else
	{
		changes.push_back ((char)action + HAS_COPY_FROM);

		// add fromPath info

		CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
		copyFromPaths.push_back (parsedFromPath.GetIndex());
		copyFromRevisions.push_back (fromRevision);

		++(*copyFromOffsets.rbegin());
	}
}

void CRevisionInfoContainer::AddMergedRevision ( const std::string& fromPath
											   , const std::string& toPath
											   , revision_t revisionStart
											   , revision_t revisionDelta)
{
	assert (revisionDelta != 0);

	// under x64, there might actually be an overflow

	if (changes.size() == NO_INDEX)
		throw std::exception ("revision container change list overflow");

	// another change

	++(*mergedRevisionsOffsets.rbegin());

	// add merged revision range and path info

	CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
	mergedFromPaths.push_back (parsedFromPath.GetIndex());
	CDictionaryBasedPath parsedToPath (&paths, toPath, false);
	mergedToPaths.push_back (parsedToPath.GetIndex());

	mergedRangeStarts.push_back (revisionStart);
	mergedRangeDeltas.push_back (revisionDelta);
}

// reset content

void CRevisionInfoContainer::Clear()
{
	authorPool.Clear();
	paths.Clear();
	comments.Clear();

	authors.clear();
	timeStamps.clear();

	rootPaths.clear();

	changesOffsets.erase (changesOffsets.begin()+1, changesOffsets.end());
	copyFromOffsets.erase (copyFromOffsets.begin()+1, copyFromOffsets.end());
	mergedRevisionsOffsets.erase (mergedRevisionsOffsets.begin()+1, mergedRevisionsOffsets.end());

	changes.clear();
	changedPaths.clear();
	copyFromPaths.clear();
	copyFromRevisions.clear();

	mergedFromPaths.clear();
	mergedToPaths.clear();
	mergedRangeStarts.clear();
	mergedRangeDeltas.clear();
}

// update / modify existing data
// indexes must be in ascending order
// indexes[] may be size() -> results in an Append()

void CRevisionInfoContainer::Update ( const CRevisionInfoContainer& newData
									, const index_mapping_t& indexMap
									, bool updateAuthors
									, bool updateTimeStamps
									, bool updateComments
									, bool updateChanges
									, bool updateMergers)
{
	// make new paths available

	index_mapping_t pathIDMapping = paths.Merge (newData.paths);

	// replace exising data

	if (updateAuthors)
		UpdateAuthors (newData, indexMap);
	if (updateTimeStamps)
		UpdateTimeStamps (newData, indexMap);
	if (updateComments)
		UpdateComments (newData, indexMap);

	if (updateChanges)
		UpdateChanges (newData, indexMap, pathIDMapping);
	if (updateMergers)
		UpdateMergers (newData, indexMap, pathIDMapping);

	// append remaining data

	Append (newData, indexMap, pathIDMapping);
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionInfoContainer& container)
{
	// read the pools

	IHierarchicalInStream* authorPoolStream
		= stream.GetSubStream (CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID);
	*authorPoolStream >> container.authorPool;

	IHierarchicalInStream* commentsStream
		= stream.GetSubStream (CRevisionInfoContainer::COMMENTS_STREAM_ID);
	*commentsStream >> container.comments;

	IHierarchicalInStream* pathsStream
		= stream.GetSubStream (CRevisionInfoContainer::PATHS_STREAM_ID);
	*pathsStream >> container.paths;

	// read the revision info

	CDiffIntegerInStream* authorsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::AUTHORS_STREAM_ID));
	*authorsStream >> container.authors;

	CPackedTime64InStream* timeStampsStream 
		= dynamic_cast<CPackedTime64InStream*>
			(stream.GetSubStream (CRevisionInfoContainer::TIMESTAMPS_STREAM_ID));
	*timeStampsStream >> container.timeStamps;

	CDiffIntegerInStream* rootPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::ROOTPATHS_STREAM_ID));
	*rootPathsStream >> container.rootPaths;

	CDiffDWORDInStream* changesOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID));
	*changesOffsetsStream >> container.changesOffsets;

	CDiffDWORDInStream* copyFromOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID));
	*copyFromOffsetsStream >> container.copyFromOffsets;

	CDiffDWORDInStream* mergedRevisionsOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
		(stream.GetSubStream (CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID));
	*mergedRevisionsOffsetsStream >> container.mergedRevisionsOffsets;

	// read the changes info

	CPackedDWORDInStream* changesStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGES_STREAM_ID));
	*changesStream >> container.changes;

	CDiffIntegerInStream* changedPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID));
	*changedPathsStream >> container.changedPaths;

	CDiffIntegerInStream* copyFromPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID));
	*copyFromPathsStream >> container.copyFromPaths;

	CDiffIntegerInStream* copyFromRevisionsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID));
	*copyFromRevisionsStream >> container.copyFromRevisions;

	// merged revisions

	CDiffIntegerInStream* mergedFromPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID));
	*mergedFromPathsStream >> container.mergedFromPaths;

	CDiffIntegerInStream* mergedToPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID));
	*mergedToPathsStream >> container.mergedToPaths;

	CDiffIntegerInStream* mergedRangeStartsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID));
	*mergedRangeStartsStream >> container.mergedRangeStarts;

	CDiffIntegerInStream* mergedRangeDeltasStream
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID));
	*mergedRangeDeltasStream >> container.mergedRangeDeltas;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionInfoContainer& container)
{
	// write the pools

	IHierarchicalOutStream* authorPoolStream
		= stream.OpenSubStream ( CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*authorPoolStream << container.authorPool;

	IHierarchicalOutStream* commentsStream
		= stream.OpenSubStream ( CRevisionInfoContainer::COMMENTS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*commentsStream << container.comments;

	IHierarchicalOutStream* pathsStream
		= stream.OpenSubStream ( CRevisionInfoContainer::PATHS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*pathsStream << container.paths;

	// write the revision info

	CDiffIntegerOutStream* authorsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::AUTHORS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*authorsStream << container.authors;

	CPackedTime64OutStream* timeStampsStream 
		= dynamic_cast<CPackedTime64OutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::TIMESTAMPS_STREAM_ID
								  , PACKED_TIME64_STREAM_TYPE_ID));
	*timeStampsStream << container.timeStamps;

	CDiffIntegerOutStream* rootPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::ROOTPATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*rootPathsStream << container.rootPaths;

	CDiffDWORDOutStream* changesOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*changesOffsetsStream << container.changesOffsets;

	CDiffDWORDOutStream* copyFromOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*copyFromOffsetsStream << container.copyFromOffsets;

	CDiffDWORDOutStream* mergedRevisionsOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*mergedRevisionsOffsetsStream << container.mergedRevisionsOffsets;

	// write the changes info

	CPackedDWORDOutStream* changesStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGES_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));
	*changesStream << container.changes;

	CDiffIntegerOutStream* changedPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*changedPathsStream << container.changedPaths;

	CDiffIntegerOutStream* copyFromPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*copyFromPathsStream << container.copyFromPaths;

	CDiffIntegerOutStream* copyFromRevisionsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*copyFromRevisionsStream << container.copyFromRevisions;

	// merged revisions

	CDiffIntegerOutStream* mergedFromPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedFromPathsStream << container.mergedFromPaths;

	CDiffIntegerOutStream* mergedToPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedToPathsStream << container.mergedToPaths;

	CDiffIntegerOutStream* mergedRangeStartsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedRangeStartsStream << container.mergedRangeStarts;

	CDiffIntegerOutStream* mergedRangeDeltasStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedRangeDeltasStream << container.mergedRangeDeltas;

	// ready

	return stream;
}

// end namespace LogCache

}
