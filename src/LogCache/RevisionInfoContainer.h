#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "PathDictionary.h"
#include "TokenizedStringContainer.h"

///////////////////////////////////////////////////////////////
//
// CRevisionInfoContainer
//
//		stores all log information except the actual version
//		number. So, please note, that the indices used here
//		are not the revision number (revisons may come in
//		in arbitrary order while new information will only
//		be appended here).
//
//		Every entry holds the data for exactly one revision.
//		You may add change paths to the last revision, only.
//
//		Internal storage for revision "index" is as follows:
//
//			* comments[index], authors[authors[index]] and
//			  timeStamps[index] contain the values passed to 
//			  Insert()
//
//			* CDictionaryBasedPath (&paths, rootPaths[index])
//			  is the common root path for all changed paths
//			  in that revision (returns true in IsInvalid(),
//			  if there are no changed paths for this revision).
//
//			* changesOffsets[index] .. changesOffsets[index+1]-1
//			  is the range within changes and changedPaths 
//			  that contains all changes to the revision
//
//			* copyFromOffsets[index] .. copyFromOffsets[index+1]-1
//			  is the corresponding range within copyFromPaths 
//			  and copyFromRevisions 
//
//		changes contains the TChangeAction values. If a non-
//		empty fromPath has been passed to AddChange(), "1" is
//		added to the action value. Only in that case, there
//		will be entries in copyFromPaths and copyFromRevisions.
//		(so, iterators need two differen change indices to
//		represent their current position).
//
///////////////////////////////////////////////////////////////

class CRevisionInfoContainer
{
private:

	// string pool for author names

	CStringDictionary authorPool;

	// common directory for all paths

	CPathDictionary paths;

	// comment, author and timeStamp per revision index

	CTokenizedStringContainer comments;
	std::vector<DWORD> authors;
	std::vector<DWORD> timeStamps;

	// common root of all changed paths in this revision

	std::vector<DWORD> rootPaths;

	// mark the ranges that contain the changed path info

	std::vector<DWORD> changesOffsets;
	std::vector<DWORD> copyFromOffsets;

	// changed path info
	// (note, that copyFrom info will have less entries)

	std::vector<unsigned char> changes;
	std::vector<DWORD> changedPaths;
	std::vector<DWORD> copyFromPaths;
	std::vector<DWORD> copyFromRevisions;

	// sub-stream IDs

	enum
	{
		AUTHOR_POOL_STREAM_ID = 1,
		COMMENTS_STREAM_ID = 2,
		PATHS_STREAM_ID = 3,
		AUTHORS_STREAM_ID = 4,
		TIMESTAMPS_STREAM_ID = 5,
		ROOTPATHS_STREAM_ID = 6,
		CHANGES_OFFSETS_STREAM_ID = 7,
		COPYFROM_OFFSETS_STREAM_ID = 8,
		CHANGES_STREAM_ID = 9,
		CHANGED_PATHS_STREAM_ID = 10,
		COPYFROM_PATHS_STREAM_ID = 11,
		COPYFROM_REVISIONS_STREAM_ID = 12
	};

	// index checking utility

	void CheckIndex (size_t index) const
	{
		if (index >= size())
			throw std::exception ("revision info index out of range");
	}

public:

	///////////////////////////////////////////////////////////////
	//
	// TChangeAction
	//
	//		the action IDs to use with AddChange 
	//		(actually, a bit mask).
	//
	///////////////////////////////////////////////////////////////

	enum TChangeAction
	{
		ACTION_ADDED	= 0x04,
		ACTION_CHANGED	= 0x08,
		ACTION_REPLACED	= 0x10,
		ACTION_DELETED	= 0x20,

		ANY_ACTION		= ACTION_ADDED 
						| ACTION_CHANGED
						| ACTION_REPLACED
						| ACTION_DELETED
	};

	///////////////////////////////////////////////////////////////
	//
	// CChangesIterator
	//
	//		a very simplistic forward iterator class.
	//		It will be used to provide a convenient
	//		interface to a revision's changes list.
	//
	///////////////////////////////////////////////////////////////

	class CChangesIterator
	{
	private:

		// the container we operate on 

		const CRevisionInfoContainer* container;

		// the two different change info indices

		size_t changeOffset;
		size_t copyFromOffset;

	public:

		// construction

		CChangesIterator()
			: container (NULL)
			, changeOffset(0)
			, copyFromOffset(0)
		{
		}

		CChangesIterator ( const CRevisionInfoContainer* aContainer
						 , size_t aChangeOffset
						 , size_t aCopyFromOffset)
			: container (aContainer)
			, changeOffset (aChangeOffset)
			, copyFromOffset (aCopyFromOffset)
		{
			assert (IsValid());
		}

		// data access

		CRevisionInfoContainer::TChangeAction GetAction() const
		{
			assert (IsValid());
			int action = container->changes[changeOffset] & ANY_ACTION;
			return (CRevisionInfoContainer::TChangeAction)(action);
		}

		bool HasFromPath() const
		{
			assert (IsValid());
			return (container->changes[changeOffset] & ~ANY_ACTION) != 0;
		}

		CDictionaryBasedPath GetPath() const
		{
			assert (IsValid());
			size_t pathID = container->changedPaths[changeOffset];
			return CDictionaryBasedPath (&container->paths, pathID);
		}

		CDictionaryBasedPath GetFromPath() const
		{
			assert (HasFromPath());
			size_t pathID = container->copyFromPaths [copyFromOffset];
			return CDictionaryBasedPath (&container->paths, pathID);
		}

		DWORD GetFromRevision() const
		{
			assert (HasFromPath());
			return container->copyFromRevisions [copyFromOffset];
		}

		// general status (points to an action)

		bool IsValid() const
		{
			return (container != NULL)
				&& (changeOffset < container->changes.size())
				&& (copyFromOffset <= container->copyFromPaths.size());
		}

		// move pointer

		CChangesIterator& operator++()		// prefix
		{
			if (HasFromPath())
				++copyFromOffset;
			++changeOffset;

			return *this;
		}

		CChangesIterator operator++(int)	// postfix
		{
			CChangesIterator result (*this);
			operator++();
			return result;
		}

		// comparison

		bool operator== (const CChangesIterator& rhs)
		{
			return (container == rhs.container)
				&& (changeOffset == rhs.changeOffset);
		}
		bool operator!= (const CChangesIterator& rhs)
		{
			return !operator==(rhs);
		}
	};

	friend class CChangesIterator;

	// construction / destruction

	CRevisionInfoContainer(void);
	~CRevisionInfoContainer(void);

	// add information
	// AddChange() always adds to the last revision

	size_t Insert ( const std::string& author
				  , const std::string& comment
				  , DWORD timeStamp);

	void AddChange ( TChangeAction action
				   , const std::string& path
				   , const std::string& fromPath
				   , DWORD fromRevision);

	// get information

	size_t size() const
	{
		return authors.size();
	}

	const char* GetAuthor (size_t index) const
	{
		CheckIndex (index);
		return authorPool [authors [index]];
	}

	DWORD GetTimeStamp (size_t index) const
	{
		CheckIndex (index);
		return timeStamps [index];
	}

	std::string GetComment (size_t index) const
	{
		CheckIndex (index);
		return comments [index];
	}

	CDictionaryBasedPath GetRootPath (size_t index) const
	{
		CheckIndex (index);
		return CDictionaryBasedPath (&paths, rootPaths [index]);
	}

	// iterate over all changes

	CChangesIterator GetChangesBegin (size_t index) const
	{
		CheckIndex (index);
		return CChangesIterator ( this
								, changesOffsets[index]
								, copyFromOffsets[index]);
	}

	CChangesIterator GetChangesEnd (size_t index) const
	{
		CheckIndex (index);
		return CChangesIterator ( this
								, changesOffsets[index+1]
								, copyFromOffsets[index+1]);
	}

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CRevisionInfoContainer& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CRevisionInfoContainer& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionInfoContainer& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionInfoContainer& container);

