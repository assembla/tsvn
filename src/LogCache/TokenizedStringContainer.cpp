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
#include ".\tokenizedstringcontainer.h"

#include "PackedIntegerInStream.h"
#include "PackedIntegerOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"
#include "CompositeOutStream.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CPairPacker
///////////////////////////////////////////////////////////////

// find all 

void CTokenizedStringContainer::CPairPacker::Initialize()
{
	strings.reserve (container->offsets.size());
	newPairs.reserve (container->pairs.size());
	counts.reserve (container->pairs.size());

	IT first = container->offsets.begin();
	IT last = container->offsets.end();
	IT iter = first;

	assert (first != last);

	index_t offset = *iter;
	for (++iter; iter != last; ++iter)
	{
		index_t nextOffset = *iter;
		if (offset + 2 <= nextOffset)
			strings.push_back ((index_t)(iter - first - 1));

		offset = nextOffset;
	}
}

// efficiently determine the iterator range that spans our string

void CTokenizedStringContainer::CPairPacker::GetStringRange ( index_t stringIndex
															, IT& first
															, IT& last)
{

	IT iter = container->offsets.begin() + stringIndex;

	size_t offset = *iter;
	size_t length = *(++iter) - offset;

	IT base = container->stringData.begin();
	first = base + offset;
	last = first + length;
}

// add token pairs of one string to our counters

void CTokenizedStringContainer::CPairPacker::AccumulatePairs (index_t stringIndex)
{
	IT iter;
	IT end;

	GetStringRange (stringIndex, iter, end);

	index_t lastToken = *iter;
	for ( ++iter; (iter != end) && container->IsToken (*iter); ++iter)
	{
		index_t token = *iter;
		if ((token >= minimumToken) || (lastToken >= minimumToken))
		{
			// that pair could be compressible (i.e. wasn't tried before)

			std::pair<index_t, index_t> newPair (lastToken, token);
			assert (container->pairs.Find (newPair) == NO_INDEX);

			index_t index = newPairs.AutoInsert (newPair);
			if (index >= (index_t)counts.size())
				counts.push_back (1);
			else
				++counts[index];
		}

		lastToken = token;
	}
}

void CTokenizedStringContainer::CPairPacker::AddCompressablePairs()
{
	for (index_t i = 0, count = (index_t)counts.size(); i < count; ++i)
	{
		if (counts[i] > 2)
			container->pairs.Insert (newPairs[i]);
	}
}

bool CTokenizedStringContainer::CPairPacker::CompressString (index_t stringIndex)
{
	size_t oldReplacementCount = replacements;

	IT iter;
	IT end;

	GetStringRange (stringIndex, iter, end);

	IT target = iter;

	index_t lastToken = *iter;
	for ( ++iter; (iter != end) && container->IsToken (*iter); ++iter)
	{
		// can we combine last token with this one?

		index_t token = *iter;
		if ((token >= minimumToken) || (lastToken >= minimumToken))
		{
			// that pair could be compressible (i.e. wasn't tried before)

			std::pair<index_t, index_t> newPair (lastToken, token);
			index_t pairIndex = container->pairs.Find (newPair);

			if (pairIndex != NO_INDEX)
			{
				// replace token *pair* with new, compressed token

				lastToken = container->GetPairToken (pairIndex);
				if (++iter == end)
				{
					token = EMPTY_TOKEN;
					--iter;
				}
				else
					token = *iter;

				++replacements;
			}
		}

		// write to disk

		*target = lastToken;
		++target;

		lastToken = token;
	}

	// store the last token in fly

	*target = lastToken;

	// fill up the empty space at the end

	for (++target; (target != end); ++target)
		*target = EMPTY_TOKEN;
	
	// was there a compression?

	return replacements > oldReplacementCount;
}

CTokenizedStringContainer::CPairPacker::CPairPacker 
	( CTokenizedStringContainer* aContainer)
	: container (aContainer)
	, minimumToken (0)
	, replacements(0)
{
	Initialize();
}

CTokenizedStringContainer::CPairPacker::~CPairPacker()
{
}

// perform one round of compression and return the number of tokens saved

size_t CTokenizedStringContainer::CPairPacker::OneRound()
{
	// save current state

	index_t oldReplacementCount = replacements;
	index_t oldPairsCount = container->pairs.size();

	// count pairs

	for (IT iter = strings.begin(), end = strings.end(); iter != end; ++iter)
		AccumulatePairs (*iter);

	// add new, compressible pairs to the container

	AddCompressablePairs();

	// compress strings

	IT target = strings.begin();
	for (IT iter = target, end = strings.end(); iter != end; ++iter)
	{
		index_t stringIndex = *iter;
		if (CompressString (stringIndex))
		{
			*target = stringIndex;
			++target;
		}
	}

	// keep only those we might compress further

	strings.erase (target, strings.end());

	// update internal status
	// (next round can find new pairs only with tokens we just added)

	newPairs.Clear();
	counts.clear();
	minimumToken = container->GetPairToken (oldPairsCount);

	// report our results

	return replacements - oldReplacementCount;
}

void CTokenizedStringContainer::CPairPacker::Compact()
{
	if (replacements == 0)
		return;

	IT first = container->stringData.begin();
	IT target = first;

	IT offsetIter = container->offsets.begin();
	IT offsetsEnd = container->offsets.end();
	IT nextStringStart = first + *offsetIter;

	IT iter = target;
	IT end = container->stringData.end();

	while (iter != end)
	{
		// update string boundaries 
		// (use "while" loop to handle empty strings)

		while (iter == nextStringStart)
		{
			*offsetIter = (index_t)(target - first);
			nextStringStart = first + *(++offsetIter);
		}

		// copy string token

		if (container->IsToken (*iter))
		{
			*target = *iter;
			++iter;
			++target;
		}
		else
		{
			// jump to end of string

			iter = nextStringStart;
		}
	}

	// update string boundaries 

	std::fill (offsetIter, offsetsEnd, (index_t)(target - first));

	// remove trailing tokens

	container->stringData.erase (target, end);
}

///////////////////////////////////////////////////////////////
//
// CTokenizedStringContainer
//
///////////////////////////////////////////////////////////////

// data access utility

void CTokenizedStringContainer::AppendToken ( std::string& target
											, index_t token) const
{
	if (IsDictionaryWord (token))
	{
		// uncompressed token

		target.append (words [GetWordIndex (token)]);
	}
	else
	{
		// token is a compressed pair of tokens

		std::pair<index_t, index_t> subTokens = pairs [GetPairIndex (token)];

		// add them recursively

		AppendToken (target, subTokens.first);
		AppendToken (target, subTokens.second);
	}
}

// insertion utilties

void CTokenizedStringContainer::Append (index_t token)
{
	if (IsToken (token))
	{
		if (stringData.size() == NO_INDEX)
			throw std::exception ("string container overflow");

		stringData.push_back (token);
	}
}

void CTokenizedStringContainer::Append (const std::string& s)
{
	static const std::string delimiters (" \t\n\\/");

	index_t lastToken = EMPTY_TOKEN;
	size_t nextPos = std::string::npos;

	for (size_t pos = 0, length = s.length(); pos < length; pos = nextPos)
	{
		// extract the next word / token

		nextPos = s.find_first_of (delimiters, pos+1);
		if (nextPos == std::string::npos)
			nextPos = length;

		std::string word = s.substr (pos, nextPos - pos);
		index_t token = GetWordToken (words.AutoInsert (word.c_str()));

		// auto-compress, if pair with last token is already known

		index_t pairIndex = pairs.Find (std::make_pair (lastToken, token));
		if (pairIndex == NO_INDEX)
		{
			Append (lastToken);
			lastToken = token;
		}
		else
		{
			lastToken = GetPairToken (pairIndex);
		}
	}

	// don't forget the last one 
	// (will be EMPTY_TOKEN if s is empty -> will not be added in that case)

	Append (lastToken);
}

// range check

void CTokenizedStringContainer::CheckIndex (index_t index) const
{
	if (index >= offsets.size()-1)
		throw std::exception ("string container index out of range");
}

// construction / destruction

CTokenizedStringContainer::CTokenizedStringContainer(void)
{
	offsets.push_back (0);
}

CTokenizedStringContainer::~CTokenizedStringContainer(void)
{
}

// data access

std::string CTokenizedStringContainer::operator[] (index_t index) const
{
	// range check

	CheckIndex (index);

	// the iterators over the (compressed) tokens 
	// to buid the string from

	TSDIterator first = stringData.begin() + offsets[index];
	TSDIterator last = stringData.begin() + offsets[index+1];

	// re-construct the string token by token

	std::string result;
	for (TSDIterator iter = first; (iter != last) && IsToken (*iter); ++iter)
		AppendToken (result, *iter);

	return result;
}

// modification

index_t CTokenizedStringContainer::Insert (const std::string& s)
{
	// tokenize and compress

	Append (s);

	// no error -> we can add the new string to the index

	offsets.push_back ((index_t)stringData.size());
	return (index_t)(offsets.size()-2);
}

void CTokenizedStringContainer::Remove (index_t index)
{
	// range check

	CheckIndex (index);

	// special case: last string

	if (index+2 == offsets.size())
	{
		offsets.pop_back();
		return;
	}

	// terminate the previous string 

	assert (offsets[index] != offsets[index+1]);
	stringData[offsets[index]] = EMPTY_TOKEN;

	// remove string from index but keep its tokens
	// (they remain hidden and will be removed upon
	// the next "compress" run)

	offsets.erase (offsets.begin()+index);
}

void CTokenizedStringContainer::Compress()
{
	CPairPacker packer (this);

	while (packer.OneRound() > 0);

	packer.Compact();
}

void CTokenizedStringContainer::AutoCompress()
{
	// Compress only if "too few" pairs have been found yet.

	// In typical repositories, the relation of string
	// tokens to used token pairs is ~10:1.
	
	// We will favour compressing in small caches while
	// making this expensive operation less likely for
	// larger caches with already a reasonable number of
	// token pairs. Threashold: log n > n / p

	size_t relation = stringData.size() / ((size_t)pairs.size() + 1);
	if (stringData.size() < ((size_t)1 << relation))
		Compress();
}

// reset content

void CTokenizedStringContainer::Clear()
{
	words.Clear();
	pairs.Clear();

	stringData.clear();

	offsets.erase (offsets.begin()+1, offsets.end());
}

// batch modifications
// indexes must be in ascending order

// Replace() will append new entries, 
// if indices[] matches current size().

void CTokenizedStringContainer::Remove (const std::vector<index_t>& indexes)
{
	// prepatation

	CIT indexIter = indexes.begin();
	CIT indexEnd = indexes.end();

	IT firstToken = stringData.begin();
	IT targetToken = firstToken;

	// copy & drop strings ("remove_copy_if") in-situ 

	for ( index_t i = 0, target = 0, count = (index_t)offsets.size()-1
		; i < count
		; ++i)
	{
		if ((indexIter != indexEnd) && (*indexIter == i))
		{
			// skip / remove this token string

			++indexIter;
		}
		else
		{
			// copy string tokens

			targetToken = std::copy ( firstToken + offsets[i]
								    , firstToken + offsets[i+1]
									, targetToken);

			// update (end-)offset

			offsets[++target] = static_cast<index_t>(targetToken - firstToken);
		}
	}

	// trim containers

	offsets.erase (offsets.end() - indexes.size(), offsets.end());
	stringData.erase (targetToken, stringData.end());
}

void CTokenizedStringContainer::Replace ( const CTokenizedStringContainer& source
										, const std::vector<index_t>& indexes)
{
	assert (indexes.size() == source.size());

	// we will fully rebuild the string token buffer
	// -> save the old one and replace it with an empty buffer

	std::vector<index_t> oldData;
	oldData.swap (stringData);

	IT oldFirst = oldData.begin();

	// splice the data

	index_t k = 0;
	index_t indexCount = static_cast<index_t>(indexes.size());

	for (index_t i = 0, count = (index_t)offsets.size()-1; i < count; ++i)
	{
		if ((k != indexCount) && (indexes[k] == i))
		{
			// replace this token string

			Append (source[k]);
			++k;
		}
		else
		{
			// copy string tokens

			std::copy ( oldFirst + offsets[i]
				      , oldFirst + offsets[i+1]
					  , stringData.end());
		}

		// update (end-)offset

		offsets[i+1] = static_cast<index_t>(stringData.size());
	}

	// append remaining strings

	for (; k < indexCount; ++k)
	{
		assert (indexes[k] == size());
		Insert (source[k]);
	}
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CTokenizedStringContainer& container)
{
	// read the words

	IHierarchicalInStream* wordsStream
		= stream.GetSubStream (CTokenizedStringContainer::WORDS_STREAM_ID);
	*wordsStream >> container.words;

	// read the token pairs

	IHierarchicalInStream* pairsStream
		= stream.GetSubStream (CTokenizedStringContainer::PAIRS_STREAM_ID);
	*pairsStream >> container.pairs;

	// read the strings

	CPackedIntegerInStream* stringsStream 
		= dynamic_cast<CPackedIntegerInStream*>
			(stream.GetSubStream (CTokenizedStringContainer::STRINGS_STREAM_ID));
	*stringsStream >> container.stringData;

	// read the string offsets

	CDiffDWORDInStream* offsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CTokenizedStringContainer::OFFSETS_STREAM_ID));
	*offsetsStream >> container.offsets;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CTokenizedStringContainer& container)
{
	// write the words

	CCompositeOutStream* wordsStream 
		= dynamic_cast<CCompositeOutStream*>
			(stream.OpenSubStream ( CTokenizedStringContainer::WORDS_STREAM_ID
								  , COMPOSITE_STREAM_TYPE_ID));
	const_cast<CTokenizedStringContainer*>(&container)->AutoCompress();
	*wordsStream << container.words;

	// write the pairs

	IHierarchicalOutStream* pairsStream
		= stream.OpenSubStream ( CTokenizedStringContainer::PAIRS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*pairsStream << container.pairs;

	// the strings

	CPackedIntegerOutStream* stringsStream 
		= dynamic_cast<CPackedIntegerOutStream*>
			(stream.OpenSubStream ( CTokenizedStringContainer::STRINGS_STREAM_ID
								  , PACKED_INTEGER_STREAM_TYPE_ID));
	*stringsStream << container.stringData;

	// the string positions

	CDiffDWORDOutStream* offsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CTokenizedStringContainer::OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*offsetsStream << container.offsets;

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
