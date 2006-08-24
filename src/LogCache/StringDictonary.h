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
///////////////////////////////////////////////////////////////

class CStringDictonary
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

		CStringDictonary* dictionary;

	public:

		// simple construction

		CHashFunction (CStringDictonary* aDictionary);

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

	// no assignment nor copy

	CStringDictonary (const CStringDictonary&);
	CStringDictonary& operator= (const CStringDictonary&);

	// test for the worst effects of data corruption

	void CheckOffsets();

public:

	// construction / destruction

	CStringDictonary(void);
	virtual ~CStringDictonary(void);

	// dictionary operations

	size_t find (const char* string) const;
	const char* operator[](size_t index) const;
	size_t length (size_t index) const;

	size_t insert (const char* string);

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CStringDictonary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CStringDictonary& dictionary);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CStringDictonary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CStringDictonary& dictionary);

