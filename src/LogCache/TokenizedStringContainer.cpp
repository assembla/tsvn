#include "StdAfx.h"
#include ".\tokenizedstringcontainer.h"

#include "PackedIntegerInStream.h"
#include "PackedIntegerOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPairPacker
//
///////////////////////////////////////////////////////////////

// find all 

void CTokenizedStringContainer::CPairPacker::Initialize()
{
	strings.reserve (container->offsets.size());

	UIT first = container->offsets.begin();
	UIT last = container->offsets.end();
	UIT iter = first;

	assert (first != last);

	DWORD offset = *iter;
	for (++iter; iter != last; ++iter)
	{
		DWORD nextOffset = *iter;
		if (offset + 2 <= nextOffset)
			strings.push_back ((DWORD)(iter - first - 1));

		offset = nextOffset;
	}
}

// efficiently determine the iterator range that spans our string

void CTokenizedStringContainer::CPairPacker::GetStringRange ( DWORD stringIndex
															, IIT& first
															, IIT& last)
{

	UIT iter = container->offsets.begin() + stringIndex;

	size_t offset = *iter;
	size_t length = *(++iter) - offset;

	IIT base = container->stringData.begin();
	first = base + offset;
	last = base + length;
}

// add token pairs of one string to our counters

void CTokenizedStringContainer::CPairPacker::AccumulatePairs (DWORD stringIndex)
{
	IIT iter;
	IIT end;

	GetStringRange (stringIndex, iter, end);

	int lastToken = *iter;
	for ( ++iter; (iter != end) && container->IsToken (*iter); ++iter)
	{
		int token = *iter;
		if ((token >= minimumToken) || (lastToken >= minimumToken))
		{
			// that pair could be compressible (i.e. wasn't tried before)

			std::pair<int, int> newPair (lastToken, token);
			assert (container->pairs.Find (newPair) == -1);

			size_t index = newPairs.AutoInsert (newPair);
			if (index >= counts.size())
				counts.push_back (1);
			else
				++counts[index];
		}

		lastToken = token;
	}
}

void CTokenizedStringContainer::CPairPacker::AddCompressablePairs()
{
	for (size_t i = 0, count = counts.size(); i < count; ++i)
	{
		if (counts[i] > 2)
			container->pairs.Insert (newPairs[i]);
	}
}

bool CTokenizedStringContainer::CPairPacker::CompressString (DWORD stringIndex)
{
	IIT iter;
	IIT end;

	GetStringRange (stringIndex, iter, end);

	IIT target = iter;

	int lastToken = *iter;
	for ( ++iter; (iter != end) && container->IsToken (*iter); ++iter)
	{
		// can we combine last token with this one?

		int token = *iter;
		if ((token >= minimumToken) || (lastToken >= minimumToken))
		{
			// that pair could be compressible (i.e. wasn't tried before)

			std::pair<int, int> newPair (lastToken, token);
			size_t pairIndex = container->pairs.Find (newPair);

			if (pairIndex != -1)
			{
				// replace token *pair* with new, compressed token

				lastToken = container->GetPairToken (pairIndex);
				if (++iter == end)
					--iter;

				++replacements;
			}
		}

		// write to disk

		*target = lastToken;
		++target;

		lastToken = token;
	}

	// any change at all?

	*target = lastToken;
	if (++target == end)
		return false;

	// fill up the empty space at the end

	for (; (target != end); ++target)
		*target = -1;
	
	// there was a compression

	return true;
}

CTokenizedStringContainer::CPairPacker::CPairPacker 
	( CTokenizedStringContainer* aContainer)
	: container (aContainer)
	, minimumToken (INT_MIN)
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

	size_t oldReplacementCount = replacements;
	size_t oldPairsCount = container->pairs.size();

	// count pairs

	for (UIT iter = strings.begin(), end = strings.end(); iter != end; ++iter)
		AccumulatePairs (*iter);

	// add new, compressible pairs to the container

	AddCompressablePairs();

	// compress strings

	UIT target = strings.begin();
	for (UIT iter = target, end = strings.end(); iter != end; ++iter)
	{
		DWORD stringIndex = *iter;
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
	minimumToken = container->GetPairToken (oldPairsCount);

	// report our results

	return replacements - oldReplacementCount;
}

void CTokenizedStringContainer::CPairPacker::Compact()
{
	if (replacements == 0)
		return;

	IIT first = container->stringData.begin();
	IIT target = first;

	UIT offsetIter = container->offsets.begin();
	IIT nextStringStart = first + *offsetIter;

	for ( IIT iter = target, end = container->stringData.end()
		; iter != end
		; ++iter)
	{
		if (iter == nextStringStart)
		{
			*offsetIter = (DWORD)(target - first);
			nextStringStart = first + *(++offsetIter);
		}

		if (container->IsToken (*iter))
		{
			*target = *iter;
			++target;
		}
	}

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
											, int token) const
{
	if (IsDictionaryWord (token))
	{
		// uncompressed token

		target.append (words [GetWordIndex (token)]);
	}
	else
	{
		// token is a compressed pair of tokens

		std::pair<int, int> subTokens = pairs [GetPairIndex (token)];

		// add them recursively

		AppendToken (target, subTokens.first);
		AppendToken (target, subTokens.second);
	}
}

// insertion utilties

void CTokenizedStringContainer::Append (int token)
{
	if (IsToken (token))
	{
		if (stringData.size() == (DWORD)(-1))
			throw std::exception ("string container overflow");

		stringData.push_back (token);
	}
}

void CTokenizedStringContainer::Append (const std::string& s)
{
	static const std::string delimiters (" \t\n\\/");

	int lastToken = EMPTY_TOKEN;
	size_t nextPos = std::string::npos;

	for (size_t pos = 0, length = s.length(); pos < length; pos = nextPos)
	{
		// extract the next word / token

		nextPos = s.find_first_of (delimiters, pos+1);
		if (nextPos == std::string::npos)
			nextPos = length;

		std::string word = s.substr (pos, nextPos - pos);
		int token = GetWordToken (words.AutoInsert (word.c_str()));

		// auto-compress, if pair with last token is already known

		size_t pairIndex = pairs.Find (std::make_pair (lastToken, token));
		if (pairIndex == -1)
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

// construction / destruction

CTokenizedStringContainer::CTokenizedStringContainer(void)
{
	offsets.push_back (0);
}

CTokenizedStringContainer::~CTokenizedStringContainer(void)
{
}

// data access

std::string CTokenizedStringContainer::operator[] (size_t index) const
{
	// range check

	if (index >= offsets.size()-1)
		throw std::exception ("string container index out of range");

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

size_t CTokenizedStringContainer::Insert (const std::string& s)
{
	// tokenize and compress

	Append (s);

	// no error -> we can add the new string to the index

	offsets.push_back ((DWORD)stringData.size());
	return offsets.size()-2;
}

void CTokenizedStringContainer::Compress()
{
	CPairPacker packer (this);

	while (packer.OneRound() > 0);

	packer.Compact();
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

	CDiffIntegerInStream* offsetsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CTokenizedStringContainer::OFFSETS_STREAM_ID));
	*offsetsStream >> container.offsets;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CTokenizedStringContainer& container)
{
	// write the words

	IHierarchicalOutStream* wordsStream 
		= stream.OpenSubStream ( CTokenizedStringContainer::WORDS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
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
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*stringsStream << container.stringData;

	// the string positions

	CDiffIntegerOutStream* offsetsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CTokenizedStringContainer::OFFSETS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*offsetsStream << container.offsets;

	// ready

	return stream;
}
