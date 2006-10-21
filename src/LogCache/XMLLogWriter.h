#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "CachedLogInfo.h"

///////////////////////////////////////////////////////////////
//
// CXMLLogReader
//
///////////////////////////////////////////////////////////////

class CBufferedOutFile;

class CXMLLogWriter
{
private:

	// for convenience

	typedef CRevisionInfoContainer::TChangeAction TChangeAction;
	typedef CRevisionInfoContainer::CChangesIterator CChangesIterator;

	static void WriteTimeStamp ( CBufferedOutFile& file
							   , __time64_t timeStamp);

	static void WriteChanges ( CBufferedOutFile& file
							 , CChangesIterator& iter	
							 , CChangesIterator& last);

	static void WriteRevisionInfo ( CBufferedOutFile& file
								  , const CRevisionInfoContainer& info
								  , DWORD revision
								  , DWORD index);

public:

	// map file to memory, parse it and fill the target

	static void SaveToXML ( const std::wstring& xmlFileName
						  , const CCachedLogInfo& source);
};

