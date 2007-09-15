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
#include ".\stringdictonary.h"

#include "BLOBInStream.h"
#include "BLOBOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CStringDictionary::CHashFunction
///////////////////////////////////////////////////////////////

// simple construction

CStringDictionary::CHashFunction::CHashFunction (CStringDictionary* aDictionary)
	: dictionary (aDictionary)
{
}

// the actual hash function

size_t 
CStringDictionary::CHashFunction::operator() 
	(CStringDictionary::CHashFunction::value_type value) const
{
	if (value == NULL)
		return 0;

	size_t result = 0;
	while (*value)
	{
		result = result * 33 ^ *value;
		++value;
	}

	return result;
}

// dictionary lookup

CStringDictionary::CHashFunction::value_type 
CStringDictionary::CHashFunction::value 
	(CStringDictionary::CHashFunction::index_type index) const
{
	if (index == NO_INDEX)
		return NULL;

	assert (dictionary->offsets.size() > index+1);
	assert (dictionary->offsets[index] != NO_INDEX);

	return &dictionary->packedStrings.at(0) + dictionary->offsets[index];
}

// lookup and comparison

bool 
CStringDictionary::CHashFunction::equal 
	( CStringDictionary::CHashFunction::value_type value
	, CStringDictionary::CHashFunction::index_type index) const
{
	// special case

	if (value == NULL)
		return index == 0;

	// ordinary string comparison

	const char* rhs = &dictionary->packedStrings.at(0) 
					+ dictionary->offsets[index];
	return strcmp (value, rhs) == 0;
}

///////////////////////////////////////////////////////////////
// CStringDictionary
///////////////////////////////////////////////////////////////

// test for the worst effects of data corruption

void CStringDictionary::RebuildIndexes()
{
    // start of the string & offset arrays

	const char* stringBase = &packedStrings.at(0);
    std::vector<index_t>::iterator begin = offsets.begin();

    // current position in string data (i.e. first char of the current string)

    size_t offset = 0;

    // hash & index all strings

	for ( index_t i = 0, count = (index_t)offsets.size()-1; i < count; ++i)
    {
        *(begin+i) = offset;
		hashIndex.insert (stringBase + offset, i);

        offset += strlen (stringBase + offset) +1;
    }

    // "end of table" entry

  	*offsets.rbegin() = (index_t)packedStrings.size();
}

// construction utility

void CStringDictionary::Initialize()
{
	// insert the empty string at index 0

	packedStrings.push_back (0);
	offsets.push_back (0);
	offsets.push_back (1);
	hashIndex.insert ("", 0);
}

// construction / destruction

CStringDictionary::CStringDictionary(void)
	: hashIndex (CHashFunction (this))
{
	Initialize();
}

CStringDictionary::~CStringDictionary(void)
{
}

// dictionary operations

index_t CStringDictionary::Find (const char* string) const
{
	return hashIndex.find (string);
}

const char* CStringDictionary::operator[](index_t index) const
{
	if (index+1 >= (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");

	return &packedStrings.at(0) + offsets[index];
}

index_t CStringDictionary::GetLength (index_t index) const
{
	if (index+1 >= (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");

	return offsets[index+1] - offsets[index] - 1;
}

index_t CStringDictionary::Insert (const char* string)
{
	// must not exist yet (so, it is not empty as well)

	assert (Find (string) == NO_INDEX);
	assert (strlen (string) < NO_INDEX);

	// add string to container

	index_t size = index_t (strlen (string)) +1;
	if (packedStrings.size() >= NO_INDEX - size)
		throw std::exception ("dictionary overflow");

	packedStrings.insert (packedStrings.end(), string, string + size);

	// update indices

	index_t result = (index_t)offsets.size()-1;
	hashIndex.insert (string, result);
	offsets.push_back ((index_t)packedStrings.size());

	// ready

	return result;
}

index_t CStringDictionary::AutoInsert (const char* string)
{
	index_t result = Find (string);
	if (result == NO_INDEX)
		result = Insert (string);

	return result;
}

// reset content

void CStringDictionary::Clear()
{
	packedStrings.clear();
	offsets.clear();
	hashIndex.clear();

	Initialize();
}

// "merge" with another container:
// add new entries and return ID mapping for source container

index_mapping_t CStringDictionary::Merge (const CStringDictionary& source)
{
	index_mapping_t result;

	for (index_t i = 0, count = source.size(); i < count; ++i)
		result.insert (i, AutoInsert (source[i]));

	return result;
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CStringDictionary& dictionary)
{
	// read the string data

	CBLOBInStream* packedStringStream 
		= dynamic_cast<CBLOBInStream*>
			(stream.GetSubStream (CStringDictionary::PACKED_STRING_STREAM_ID));

	if (packedStringStream->GetSize() >= NO_INDEX)
		throw std::exception ("data stream to large");

	dictionary.packedStrings.resize (packedStringStream->GetSize());
	memcpy ( &dictionary.packedStrings.at(0)
		   , packedStringStream->GetData()
		   , dictionary.packedStrings.size());

	// build the hash and string offsets

	CDiffDWORDInStream* offsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CStringDictionary::OFFSETS_STREAM_ID));

    size_t offsetCount = offsetsStream->GetSizeValue();
    dictionary.offsets.resize (offsetCount);

	dictionary.hashIndex 
		= quick_hash<CStringDictionary::CHashFunction>
			(CStringDictionary::CHashFunction (&dictionary));
	dictionary.hashIndex.reserve (dictionary.offsets.size());

    dictionary.RebuildIndexes();

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CStringDictionary& dictionary)
{
	// write string data

	CBLOBOutStream* packedStringStream 
		= dynamic_cast<CBLOBOutStream*>
			(stream.OpenSubStream ( CStringDictionary::PACKED_STRING_STREAM_ID
								  , BLOB_STREAM_TYPE_ID));
	packedStringStream->Add ((const unsigned char*)&dictionary.packedStrings.at(0)
							, dictionary.packedStrings.size());

	// write offsets

	CDiffDWORDOutStream* offsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CStringDictionary::OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	offsetsStream->AddSizeValue (dictionary.offsets.size());

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
