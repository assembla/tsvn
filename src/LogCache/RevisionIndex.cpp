#include "StdAfx.h"
#include ".\revisionindex.h"

#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

// construction / destruction

CRevisionIndex::CRevisionIndex(void)
	: firstRevision(0)
{
}

CRevisionIndex::~CRevisionIndex(void)
{
}

// insert info (must be -1 before)

void CRevisionIndex::SetRevisionIndex (size_t revision, DWORD index)
{
	// parameter check

	assert (operator[](revision) == -1);
	assert (index != -1);

	if (revision == -1)
		throw std::exception ("Invalid revision");

	// special cases

	if (indices.empty())
	{
		indices.push_back (index);
		firstRevision = revision;
		return;
	}

	if (revision == GetLastRevision())
	{
		indices.push_back (index);
		return;
	}

	// make sure, there is an entry in indices for that revision

	if (revision < firstRevision)
	{
		// efficiently grow on the lower end

		size_t newFirstRevision = firstRevision < indices.size()
			? 0
			: min (firstRevision - indices.size(), revision);

		indices.insert (indices.begin(), firstRevision - newFirstRevision, (DWORD)-1);
		firstRevision = newFirstRevision;
	}
	else
	{
		size_t size = indices.size();
		if (revision - firstRevision > size)
		{
			// efficently grow on the upper end

			size_t toAdd = max (size, revision - firstRevision - size);
			indices.insert (indices.end(), toAdd, (DWORD)-1);
		}
	}

	// bucked exists -> just write the value

	indices [revision - firstRevision] = index;
}

// reset content

void CRevisionIndex::Clear()
{
	indices.clear();

	firstRevision = 0;
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionIndex& container)
{
	// read the string offsets

	CDiffIntegerInStream* indexStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionIndex::INDEX_STREAM_ID));

	container.firstRevision = indexStream->GetValue();
	*indexStream >> container.indices;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionIndex& container)
{
	// the string positions

	CDiffIntegerOutStream* indexStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionIndex::INDEX_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	indexStream->Add ((int)container.firstRevision);
	*indexStream << container.indices;

	// ready

	return stream;
}
