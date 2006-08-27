#include "StdAfx.h"
#include ".\pathdictionary.h"

#include "HierachicalInStreamBase.h"
#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CPathDictionary
//
///////////////////////////////////////////////////////////////

// index check utility

void CPathDictionary::CheckParentIndex (size_t index) const
{
	if (index >= paths.size())
		throw std::exception ("parent path index out of range");
}

// construction (create root path) / destruction

CPathDictionary::CPathDictionary()
{
	paths.Insert (std::make_pair (0, 0));
}

CPathDictionary::~CPathDictionary(void)
{
}

// dictionary operations

size_t CPathDictionary::GetParent (size_t index) const
{
	return paths[index].first;
}

const char* CPathDictionary::GetPathElement (size_t index) const
{
	return pathElements [paths [index].second];
}

size_t CPathDictionary::Find (size_t parent, const char* pathElement) const
{
	size_t pathElementIndex = pathElements.Find (pathElement);
	return pathElementIndex == -1
		? -1
		: paths.Find (std::make_pair ((int)parent, (int)pathElementIndex));
}

size_t CPathDictionary::Insert (size_t parent, const char* pathElement)
{
	CheckParentIndex (parent);

	size_t pathElementIndex = pathElements.AutoInsert (pathElement);
	return paths.Insert (std::make_pair ((int)parent, (int)pathElementIndex));
}

size_t CPathDictionary::AutoInsert (size_t parent, const char* pathElement)
{
	CheckParentIndex (parent);

	size_t pathElementIndex = pathElements.AutoInsert (pathElement);
	return paths.AutoInsert (std::make_pair ( (int)parent
											, (int)pathElementIndex));
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CPathDictionary& dictionary)
{
	// read the path elements

	IHierarchicalInStream* elementsStream
		= stream.GetSubStream (CPathDictionary::ELEMENTS_STREAM_ID);
	*elementsStream >> dictionary.pathElements;

	// read the second elements

	IHierarchicalInStream* pathsStream
		= stream.GetSubStream (CPathDictionary::PATHS_STREAM_ID);
	*pathsStream >> dictionary.paths;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CPathDictionary& dictionary)
{
	// write path elements

	IHierarchicalOutStream* elementsStream 
		= stream.OpenSubStream ( CPathDictionary::ELEMENTS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*elementsStream << dictionary.pathElements;

	// write paths

	IHierarchicalOutStream* pathsStream
		= stream.OpenSubStream ( CPathDictionary::PATHS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*pathsStream << dictionary.paths;

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
//
// CDictionaryBasedPath
//
///////////////////////////////////////////////////////////////

// construction: lookup and optionally auto-insert

CDictionaryBasedPath::CDictionaryBasedPath ( CPathDictionary* aDictionary
										   , const std::string& path
										   , bool nextParent)
	: dictionary (aDictionary)
	, index (0)
{
	if (!path.empty())
	{
		std::string temp (path);
		assert (path[0] == '/');

		for ( size_t pos = 0, nextPos = temp.find ('/', 1)
			; pos != std::string::npos
			; pos = nextPos, nextPos = temp.find ('/', nextPos))
		{
			// get the current path element and terminate it properly

			const char* pathElement = temp.c_str() + pos+1;
			if (nextPos != std::string::npos)
				temp[nextPos] = 0;

			// try move to the next sub-path

			size_t nextIndex = dictionary->Find (index, pathElement);
			if (nextIndex == -1)
			{
				// not found. Stop here?

				if (nextParent)
					break;

				// auto-insert

				nextIndex = aDictionary->Insert (index, pathElement);
			}

			// we are now one level deeper

			index = nextIndex;
		}
	}
}

std::string CDictionaryBasedPath::GetPath() const
{
	// fetch all path elements bottom-up except the root

	std::vector<const char*> pathElements;
	pathElements.reserve (15);

	for ( size_t currentIndex = index
		; index != 0
		; currentIndex = dictionary->GetParent (currentIndex))
	{
		pathElements.push_back (dictionary->GetPathElement (currentIndex));
	}

	// calculate the total string length

	size_t size = pathElements.size();
	for (size_t i = 0, count = pathElements.size(); i < count; ++i)
		size += strlen (pathElements[i]);

	// build result

	std::string result;
	result.reserve (size);

	for (size_t i = pathElements.size(); i > 0; --i)
	{
		result.push_back ('/');
		result.append (pathElements[i-1]);
	}

	// special case: the root

	if (result.empty())
		result.push_back ('/');

	// ready

	return result;
}

CDictionaryBasedPath CDictionaryBasedPath::GetCommonRoot (size_t rhsIndex) const
{
	assert ((index != (-1)) && (rhsIndex != (-1)));

	size_t lhsIndex = index;

	while (lhsIndex != rhsIndex)
	{
		// the parent has *always* a smaller index
		// -> a common parent cannot be larger than lhs or rhs

		if (lhsIndex < rhsIndex)
		{
			rhsIndex = dictionary->GetParent (rhsIndex);
		}
		else
		{
			lhsIndex = dictionary->GetParent (lhsIndex);
		}
	}

	return CDictionaryBasedPath (dictionary, lhsIndex);
}
