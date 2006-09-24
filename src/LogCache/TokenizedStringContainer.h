#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "IndexPairDictionary.h"

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

		typedef std::vector<int>::iterator IIT;
		typedef std::vector<DWORD>::iterator UIT;

		// the container we operate on and the indicies
		// of all strings that might be compressed

		CTokenizedStringContainer* container;
		std::vector<DWORD> strings;

		// the pairs we found and the number of places they occur in

		CIndexPairDictionary newPairs;
		std::vector<DWORD> counts;

		// tokens smaller than that cannot be compressed
		// (internal optimization as after the initial round 
		// only combinations with the latest pairs may yield
		// further pairs)

		int minimumToken;

		// total number of replacements performed so far
		// (i.e. number of tokens saved)

		size_t replacements;

		// find all strings that consist of more than one token

		void Initialize();

		// efficiently determine the iterator range that spans our string

		void GetStringRange (DWORD stringIndex, IIT& first, IIT& last);

		// add token pairs of one string to our counters

		void AccumulatePairs (DWORD stringIndex);
		void AddCompressablePairs();
		bool CompressString (DWORD stringIndex);

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

	typedef std::vector<int>::const_iterator TSDIterator;

	// the token contents: words and pairs

	CStringDictionary words;
	CIndexPairDictionary pairs;

	// container for all tokens of all strings

	std::vector<int> stringData;

	// marks the ranges within stringData that form the strings

	std::vector<DWORD> offsets;

	// sub-stream IDs

	enum
	{
		WORDS_STREAM_ID = 1,
		PAIRS_STREAM_ID = 2,
		STRINGS_STREAM_ID = 3,
		OFFSETS_STREAM_ID = 4
	};

	// token coding

	enum {EMPTY_TOKEN = -1};

	bool IsToken (int token) const
	{
		return token != EMPTY_TOKEN;
	}
	bool IsDictionaryWord (int token) const
	{
		return token < EMPTY_TOKEN;
	}

	size_t GetWordIndex (int token) const
	{
		return -1 - token;
	}
	size_t GetPairIndex (int token) const
	{
		return token;
	}

	int GetWordToken (size_t wordIndex) const
	{
		return -1 - (int)wordIndex;
	}
	int GetPairToken (size_t pairIndex) const
	{
		return (int)pairIndex;
	}

	// data access utility

	void AppendToken (std::string& target, int token) const;

	// insertion utilties

	void Append (int token);
	void Append (const std::string& s);

public:

	// construction / destruction

	CTokenizedStringContainer(void);
	~CTokenizedStringContainer(void);

	// data access

	size_t size() const
	{
		return offsets.size() -1;
	}

	std::string operator[] (size_t index) const;

	// modification

	size_t Insert (const std::string& s);
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

