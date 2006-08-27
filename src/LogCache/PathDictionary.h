#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "IndexPairDictionary.h"

///////////////////////////////////////////////////////////////
//
// CPathDictionary
//
//		A very efficient storage for file paths. Every path
//		is decomposed into its elements (separented by '/').
//		Those elements are uniquely stored in a string dictionary.
//		Paths are stored as (parentPathID, elementID) pairs
//		with index/ID 0 designating the root path.
//
//		Parent(root) == root.
//
///////////////////////////////////////////////////////////////

class CPathDictionary
{
private:

	// sub-stream IDs

	enum
	{
		ELEMENTS_STREAM_ID = 1,
		PATHS_STREAM_ID = 2
	};

	// string pool containing all used path elements

	CStringDictionary pathElements;

	// the paths as (parent, sub-path) pairs
	// index 0 is always the root

	CIndexPairDictionary paths;

	// index check utility

	void CheckParentIndex (size_t index) const;

public:

	// construction (create root path) / destruction

	CPathDictionary();
	virtual ~CPathDictionary(void);

	// dictionary operations

	size_t size() const
	{
		return paths.size();
	}

	size_t GetParent (size_t index) const;
	const char* GetPathElement (size_t index) const;

	size_t Find (size_t parent, const char* pathElement) const;
	size_t Insert (size_t parent, const char* pathElement);
	size_t AutoInsert (size_t parent, const char* pathElement);

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CPathDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CPathDictionary& dictionary);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CPathDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CPathDictionary& dictionary);

///////////////////////////////////////////////////////////////
//
// CDictionaryBasedPath
//
///////////////////////////////////////////////////////////////

class CDictionaryBasedPath
{
private:

	// our dictionary and position within it

	const CPathDictionary* dictionary;
	size_t index;

public:

	// construction / destruction

	CDictionaryBasedPath (const CPathDictionary* aDictionary, size_t anIndex)
		: dictionary (aDictionary)
		, index (anIndex)
	{
	}

	CDictionaryBasedPath ( CPathDictionary* aDictionary
						 , const std::string& path
						 , bool nextParent = false);

	~CDictionaryBasedPath() 
	{
	}

	// data access

	size_t GetIndex() const
	{
		return index;
	}

	const CPathDictionary* GetDictionary() const
	{
		return dictionary;
	}

	// path operations

	bool IsRoot() const
	{
		return index == 0;
	}

	bool IsValid() const
	{
		return index != -1;
	}

	CDictionaryBasedPath GetParent() const
	{
		return CDictionaryBasedPath ( dictionary
									, dictionary->GetParent (index));
	}

	CDictionaryBasedPath GetCommonRoot (const CDictionaryBasedPath& rhs) const
	{
		return GetCommonRoot (rhs.index);
	}

	CDictionaryBasedPath GetCommonRoot (size_t rhsIndex) const;

	// convert to string

	std::string GetPath() const;
};
