#include "StdAfx.h"
#include ".\indexpairdictionary.h"

#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
// CIndexPairDictionary::CHashFunction
///////////////////////////////////////////////////////////////

// simple construction

CIndexPairDictionary::CHashFunction::CHashFunction 
	( CIndexPairDictionary* aDictionary)
	: dictionary (aDictionary)
{
}

///////////////////////////////////////////////////////////////
// CIndexPairDictionary
///////////////////////////////////////////////////////////////

// construction / destruction

CIndexPairDictionary::CIndexPairDictionary(void)
	: hashIndex (CHashFunction (this))
{
}

CIndexPairDictionary::~CIndexPairDictionary(void)
{
}

// dictionary operations

size_t CIndexPairDictionary::Find (const std::pair<int, int>& value) const
{
	return hashIndex.find (value);
}

size_t CIndexPairDictionary::Insert (const std::pair<int, int>& value)
{
	assert (Find (value) == -1);

	size_t result = data.size();
	hashIndex.insert (value, (DWORD)result);
	data.push_back (value);

	return result;
}

size_t CIndexPairDictionary::AutoInsert (const std::pair<int, int>& value)
{
	size_t result = Find (value);
	if (result == -1)
		result = Insert (value);

	return result;
}

void CIndexPairDictionary::Clear()
{
	data.clear();
	hashIndex.clear();
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CIndexPairDictionary& dictionary)
{
	// read the first elements of all pairs

	CDiffIntegerInStream* firstStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CIndexPairDictionary::FIRST_STREAM_ID));

	size_t count = firstStream->GetValue();
	dictionary.data.resize (count);

	for (size_t i = 0; i < count; ++i)
		dictionary.data[i].first = firstStream->GetValue();

	// read the second elements

	CDiffIntegerInStream* secondStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CIndexPairDictionary::SECOND_STREAM_ID));

	for (size_t i = 0; i < count; ++i)
		dictionary.data[i].second = secondStream->GetValue();

	// build the hash (ommit the empty string at index 0)

	dictionary.hashIndex 
		= quick_hash<CIndexPairDictionary::CHashFunction>
			(CIndexPairDictionary::CHashFunction (&dictionary));
	dictionary.hashIndex.reserve (dictionary.data.size());

	for (size_t i = 0; i < count; ++i)
		dictionary.hashIndex.insert (dictionary.data[i], (DWORD)i);

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CIndexPairDictionary& dictionary)
{
	size_t size = dictionary.data.size();

	// write string data

	CDiffIntegerOutStream* firstStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CIndexPairDictionary::FIRST_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	firstStream->Add ((int)size);
	for (size_t i = 0; i != size; ++i)
		firstStream->Add (dictionary.data[i].first);

	// write offsets

	CDiffIntegerOutStream* secondStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CIndexPairDictionary::SECOND_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	for (size_t i = 0; i != size; ++i)
		secondStream->Add (dictionary.data[i].second);

	// ready

	return stream;
}
