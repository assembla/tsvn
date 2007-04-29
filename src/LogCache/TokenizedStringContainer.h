#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "IndexPairDictionary.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
//
// CTokenizedStringContainer
//
//		A very efficient storage for text strings. Every string
//		gets tokenized into "words" (actually, word + delimiter).
//		While the words are stored in a dictionary, the stringData
//		contains the token indices (see below). A string i is
//		the part of stringData at indices offsets[i] .. offsets[i+1]-1.
//
//		The token sequences themselves are further compressed
//		by replacing common pairs of tokens with special "pair"
//		tokens. Those pairs can be combined with further tokens
//		and so on. Only pairs that have been found more than
//		twice will be replaced by a pair token.
//
//		Thus, the tokens are intepreted as follows:
//
//		-1				empty token (allowed only temporarily)
//		-2 .. MIN_INT	word dictionary index 1 .. MAX_INT-1
//		0 .. MAX_INT	pair dictionary index 0 .. MAX_INT
//		
//		A word token overflow is not possible since the
//		word dictionary can hold only 4G chars, i.e. 
//		less than 1G words. 
//		For a pair token overflow you need 6G (original) 
//		pairs, i.e. at least 6G uncompressed words. So,
//		that is impossible as well.
//
//		Strings newly added to the container will automatically
//		be compressed using the existing pair tokens. Compress()
//		should be called before writing the data to disk since
//		the new strings may allow for further compression.
//
///////////////////////////////////////////////////////////////

class CTokenizedStringContainer
{
private:

	///////////////////////////////////////////////////////////////
	//
	// CPairPacker
	//
	//		A utility class that encapsulates the compression
	//		algorithm. Note, that CTokenizedStringContainer::Insert()
	//		will not introduce new pairs but only use existing ones.
	//
	//		This class will be instantiated temporarily. A number
	//		of compression rounds are run until there is little or
	//		no further improvement.
	//
	//		Although the algorithm is quite straightforward, there
	//		is a lot of temporary data involved that can best be
	//		handled in this separate class.
	//
	//		Please note, that the container is always "maximally"
	//		compressed, i.e. all entries in pairs have actually
	//		been applied to all strings.
	//
	///////////////////////////////////////////////////////////////

	class CPairPacker
	{
	private:

		typedef std::vector<index_t>::iterator IT;

		// the container we operate on and the indicies
		// of all strings that might be compressed

		CTokenizedStringContainer* container;
		std::vector<index_t> strings;

		// the pairs we found and the number of places they occur in

		CIndexPairDictionary newPairs;
		std::vector<index_t> counts;

		// tokens smaller than that cannot be compressed
		// (internal optimization as after the initial round 
		// only combinations with the latest pairs may yield
		// further pairs)

		index_t minimumToken;

		// total number of replacements performed so far
		// (i.e. number of tokens saved)

		index_t replacements;

		// find all strings that consist of more than one token

		void Initialize();

		// efficiently determine the iterator range that spans our string

		void GetStringRange (index_t stringIndex, IT& first, IT& last);

		// add token pairs of one string to our counters

		void AccumulatePairs (index_t stringIndex);
		void AddCompressablePairs();
		bool CompressString (index_t stringIndex);

	public:

		// construction / destruction

		CPairPacker (CTokenizedStringContainer* aContainer);
		~CPairPacker();

		// perform one round of compression and return the number of tokens saved

		size_t OneRound();

		// call this after the last round to remove any empty entries

		void Compact();
	};

	friend class CPairPacker;

	typedef std::vector<index_t>::const_iterator TSDIterator;

	// the token contents: words and pairs

	CStringDictionary words;
	CIndexPairDictionary pairs;

	// container for all tokens of all strings

	std::vector<index_t> stringData;

	// marks the ranges within stringData that form the strings

	std::vector<index_t> offsets;

	// sub-stream IDs

	enum
	{
		WORDS_STREAM_ID = 1,
		PAIRS_STREAM_ID = 2,
		STRINGS_STREAM_ID = 3,
		OFFSETS_STREAM_ID = 4
	};

	// token coding

	enum 
	{
		EMPTY_TOKEN = NO_INDEX,
		LAST_PAIR_TOKEN = (index_t)NO_INDEX / (index_t)2
	};

	bool IsToken (index_t token) const
	{
		return token != EMPTY_TOKEN;
	}
	bool IsDictionaryWord (index_t token) const
	{
		return token > LAST_PAIR_TOKEN;
	}

	index_t GetWordIndex (index_t token) const
	{
		return EMPTY_TOKEN - token;
	}
	index_t GetPairIndex (index_t token) const
	{
		return token;
	}

	index_t GetWordToken (index_t wordIndex) const
	{
		return EMPTY_TOKEN - wordIndex;
	}
	index_t GetPairToken (index_t pairIndex) const
	{
		return pairIndex;
	}

	// data access utility

	void AppendToken (std::string& target, index_t token) const;

	// insertion utilties

	void Append (index_t token);
	void Append (const std::string& s);

public:

	// construction / destruction

	CTokenizedStringContainer(void);
	~CTokenizedStringContainer(void);

	// data access

	index_t size() const
	{
		return (index_t)offsets.size() -1;
	}

	std::string operator[] (index_t index) const;

	// modification

	index_t Insert (const std::string& s);
	void Compress();

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CTokenizedStringContainer& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CTokenizedStringContainer& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CTokenizedStringContainer& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CTokenizedStringContainer& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

