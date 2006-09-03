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
// CStringDictionary
//
//		efficiently stores a pool unique (UTF8) strings.
//		Each string is assigned an unique, immutable index.
//
//		Under most circumstances, O(1) lookup is provided.
//
///////////////////////////////////////////////////////////////

class CStringDictionary
{
private:

	///////////////////////////////////////////////////////////////
	//
	// CHashFunction
	//
	//		A simple string hash function that satisfies quick_hash'
	//		interface requirements.
	//
	//		NULL strings are supported and are mapped to index 0.
	//		Hence, the dictionary must contain the empty string at
	//		index 0.
	//
	///////////////////////////////////////////////////////////////

	class CHashFunction
	{
	private:

		// the dictionary we index with the hash
		// (used to map index -> value)

		CStringDictionary* dictionary;

	public:

		// simple construction

		CHashFunction (CStringDictionary* aDictionary);

		// required typedefs and constants

		typedef const char* value_type;
		typedef DWORD index_type;

		enum {NO_INDEX_VALUE = -1};

		// the actual hash function

		size_t operator() (value_type value) const;

		// dictionary lookup

		value_type value (index_type index) const;

		// lookup and comparison

		bool equal (value_type value, index_type index) const;
	};

	// sub-stream IDs

	enum
	{
		PACKED_STRING_STREAM_ID = 1,
		OFFSETS_STREAM_ID = 2
	};

	// the string data

	std::vector<char> packedStrings;
	std::vector<DWORD> offsets;

	// the string index

	quick_hash<CHashFunction> hashIndex;

	friend class CHashFunction;

	// test for the worst effects of data corruption

	void CheckOffsets();

public:

	// construction / destruction

	CStringDictionary(void);
	virtual ~CStringDictionary(void);

	// dictionary operations

	size_t size() const
	{
		return offsets.size()-1;
	}

	const char* operator[](size_t index) const;
	size_t GetLength (size_t index) const;

	size_t Find (const char* string) const;
	size_t Insert (const char* string);
	size_t AutoInsert (const char* string);

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CStringDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CStringDictionary& dictionary);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CStringDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CStringDictionary& dictionary);

