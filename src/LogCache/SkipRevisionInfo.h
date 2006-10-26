#pragma once

#include "QuickHash.h"

class CPathDictionary;
class CDictionaryBasedPath;
class CRevisionIndex;

class CSkipRevisionInfo
{
private:

	struct SPerPathRanges
	{
		typedef std::map<DWORD, DWORD> TRanges;
		TRanges ranges;
		DWORD pathID;

		// find next / previous "gap"

		DWORD FindNext (DWORD revision) const;
		DWORD FindPrevious (DWORD revision) const;

		// update / insert range

		void Add (DWORD start, DWORD size);
	};

	typedef SPerPathRanges::TRanges::iterator IT;

	class CIterComp
	{
	public:

		bool operator() (const IT& lhs, const IT& rhs) const
		{
			return lhs->first < rhs->first;
		}
	};

	friend class CHashFunction;

	class CHashFunction
	{
	private:

		// the dictionary we index with the hash
		// (used to map index -> value)

		std::vector<SPerPathRanges*>* data;

	public:

		// simple construction

		CHashFunction (std::vector<SPerPathRanges*>* aContainer)
			: data (aContainer)
		{
		}

		// required typedefs and constants

		typedef DWORD value_type;
		typedef DWORD index_type;

		enum {NO_INDEX_VALUE = -1};

		// the actual hash function

		size_t operator() (const value_type& value) const
		{
			return value;
		}

		// dictionary lookup

		const value_type& value (index_type index) const
		{
			assert (data->size() >= index);
			return (*data)[index]->pathID;
		}

		// lookup and comparison

		bool equal (const value_type& value, index_type index) const
		{
			assert (data->size() >= index);
			return (*data)[index]->pathID == value;
		}
	};

	std::vector<SPerPathRanges*> data;
	quick_hash<CHashFunction> index;

	const CPathDictionary* paths;
	const CRevisionIndex* revisions;

	// remove known revisions from the range

	void TryReduceRange (DWORD& revision, DWORD& size);

public:

	// construction / destruction

	CSkipRevisionInfo ( const CPathDictionary* aPathDictionary
					  , const CRevisionIndex* aRevisionIndex);
	~CSkipRevisionInfo(void);

	// query data (return -1, if not found)

	DWORD GetNextRevision (const CDictionaryBasedPath& path, DWORD revision) const;
	DWORD GetPreviousRevision (const CDictionaryBasedPath& path, DWORD revision) const;

	// add / remove data

	void Add (const CDictionaryBasedPath& path, DWORD revision, DWORD size);
	void Clear();

	// remove unnecessary entries

	void Compress();
};
