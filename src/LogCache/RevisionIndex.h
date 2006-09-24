#pragma once

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class IHierarchicalInStream;
class IHierarchicalOutStream;

///////////////////////////////////////////////////////////////
//
// CRevisionIndex
//
//		a simple class to map from revision number to revision
//		index.
//
//		To handle high revision counts efficiently, only that
//		range of revisions is stored that actually maps to
//		revision indices (i.e. revisions at the upper and 
//		lower ends may be ommitted).
//
///////////////////////////////////////////////////////////////

class CRevisionIndex
{
private:

	// the mapping and its bias

	size_t firstRevision;
	std::vector<DWORD> indices;

	// sub-stream IDs

	enum
	{
		INDEX_STREAM_ID = 1
	};

public:

	// construction / destruction

	CRevisionIndex(void);
	~CRevisionIndex(void);

	// range of (possibly available) revisions:
	// GetFirstRevision() .. GetLastRevision()-1

	size_t GetFirstRevision() const
	{
		return firstRevision;
	}
	size_t GetLastRevision() const
	{
		return firstRevision + indices.size();
	}

	// read

	DWORD operator[](size_t revision) const
	{
		revision -= firstRevision;
		return revision < indices.size()
			? indices[revision]
			: (DWORD)-1;
	}

	// insert info (must be -1 before)

	void SetRevisionIndex (size_t revision, DWORD index);

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CRevisionIndex& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CRevisionIndex& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionIndex& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionIndex& container);

