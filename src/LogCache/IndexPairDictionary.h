#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "QuickHash.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class IHierarchicalInStream;
class IHierarchicalOutStream;

///////////////////////////////////////////////////////////////
//
// CIndexPairDictionary
//
//		efficiently stores unique pairs of integers in a pool.
//		Each pair is assigned an unique, immutable index.
//
//		Under most circumstances, O(1) lookup is provided.
//
///////////////////////////////////////////////////////////////

class CIndexPairDictionary
{
private:

	///////////////////////////////////////////////////////////////
	//
	// CHashFunction
	//
	//		A simple string hash function that satisfies quick_hash'
	//		interface requirements.
	//
	///////////////////////////////////////////////////////////////

	class CHashFunction
	{
	private:

		// the dictionary we index with the hash
		// (used to map index -> value)

		CIndexPairDictionary* dictionary;

	public:

		// simple construction

		CHashFunction (CIndexPairDictionary* aDictionary);

		// required typedefs and constants

		typedef std::pair<int, int> value_type;
		typedef DWORD index_type;

		enum {NO_INDEX_VALUE = -1};

		// the actual hash function

		size_t operator() (const value_type& value) const
		{
			return (size_t)value.first + (size_t)value.second;
		}

		// dictionary lookup

		const value_type& value (index_type index) const
		{
			assert (dictionary->data.size() >= index);
			return dictionary->data[index];
		}

		// lookup and comparison

		bool equal (const value_type& value, index_type index) const
		{
			assert (dictionary->data.size() >= index);
			return dictionary->data[index] == value;
		}
	};

	// sub-stream IDs

	enum
	{
		FIRST_STREAM_ID = 1,
		SECOND_STREAM_ID = 2
	};

	// our data pool

	std::vector<std::pair<int, int> > data;

	// the hash index (for faster lookup)

	quick_hash<CHashFunction> hashIndex;

public:

	// construction / destruction

	CIndexPairDictionary(void);
	virtual ~CIndexPairDictionary(void);

	// dictionary operations

	size_t size() const
	{
		return data.size();
	}

	const std::pair<int, int>& operator[](size_t index) const
	{
		if (index >= data.size())
			throw std::exception ("pair dictionary index out of range");

		return data[index];
	}

	size_t Find (const std::pair<int, int>& value) const;

	size_t Insert (const std::pair<int, int>& value);

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CIndexPairDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CIndexPairDictionary& dictionary);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CIndexPairDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CIndexPairDictionary& dictionary);
