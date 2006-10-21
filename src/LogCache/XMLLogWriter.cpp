#include "StdAfx.h"
#include ".\XMLLogWriter.h"
#include ".\BufferedOutFile.h"

void CXMLLogWriter::WriteTimeStamp ( CBufferedOutFile& file
								   , __time64_t timeStamp)
{
	enum {BUFFER_SIZE = 100};
	char buffer[BUFFER_SIZE];

	int musecs = (int)(timeStamp % 1000000);
	timeStamp /= 1000000;
	const tm& time = *_gmtime64 (&timeStamp);

	_snprintf ( buffer
			  , BUFFER_SIZE
			  , "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ"
			  , time.tm_year + 1900
			  , time.tm_mon + 1
			  , time.tm_mday
			  , time.tm_hour
			  , time.tm_min
			  , time.tm_sec
			  , musecs);

	file << buffer;
}

void CXMLLogWriter::WriteChanges ( CBufferedOutFile& file
								 , CChangesIterator& iter	
								 , CChangesIterator& last)	
{
	static const std::string startText = "<path";

	static const std::string copyFromPathText = "\n   copyfrom-path=\"";
	static const std::string copyFromRevisionText = "\"\n   copyfrom-rev=\"";
	static const std::string copyFromEndText = "\"";

	static const std::string addedActionText = "\n   action=\"A\"";
	static const std::string modifiedActionText = "\n   action=\"M\"";
	static const std::string replacedActionText = "\n   action=\"R\"";
	static const std::string deletedActionText = "\n   action=\"D\"";

	static const std::string pathText = ">";
	static const std::string endText = "</path>\n";

	for (; iter != last; ++iter) 
	{
		file << startText;

		if (iter->HasFromPath())
		{
			file << copyFromPathText;
			file << iter->GetFromPath().GetPath();
			file << copyFromRevisionText;
			file << iter->GetFromRevision();
			file << copyFromEndText;
		}

		switch (iter->GetAction())
		{
			case CRevisionInfoContainer::ACTION_CHANGED:
				file << modifiedActionText;
				break;
			case CRevisionInfoContainer::ACTION_ADDED:
				file << addedActionText;
				break;
			case CRevisionInfoContainer::ACTION_DELETED:
				file << deletedActionText;
				break;
			case CRevisionInfoContainer::ACTION_REPLACED:
				file << replacedActionText;
				break;
			default:
				break;
		}

		file << pathText;
		file << iter->GetPath().GetPath();

		file << endText;
	}
}

void CXMLLogWriter::WriteRevisionInfo ( CBufferedOutFile& file
									  , const CRevisionInfoContainer& info
									  , DWORD revision
									  , DWORD index)
{
	static const std::string startText = "<logentry\n   revision=\"";
	static const std::string authorText = "\">\n<author>";
	static const std::string skipAuthorText = "\">\n<date>";
	static const std::string dateText = "</author>\n<date>";
	static const std::string pathsText = "</date>\n<paths>\n";
	static const std::string msgText = "</paths>\n<msg>";
	static const std::string endText = "</msg>\n</logentry>\n";

	file << startText;
	file << revision;

	const char* author = info.GetAuthor (index);
	if (*author == 0)
	{
		file << skipAuthorText;
	}
	else
	{
		file << authorText;
		file << author;
		file << dateText;
	}

	WriteTimeStamp (file, info.GetTimeStamp (index));

	file << pathsText;

	WriteChanges (file, info.GetChangesBegin(index), info.GetChangesEnd(index));

	file << msgText;
	file << info.GetComment (index);
	file << endText;
}

// map file to memory, parse it and fill the target

void CXMLLogWriter::SaveToXML ( const std::wstring& xmlFileName
							  , const CCachedLogInfo& source)
{
	CBufferedOutFile file (xmlFileName);

	const char* header = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<log>\n";
	const char* footer = "</log>\n";

	file << header;

	const CRevisionIndex& revisions = source.GetRevisions();
	const CRevisionInfoContainer& info = source.GetLogInfo();

	for ( size_t revision = revisions.GetLastRevision()-1
		, fristRevision = revisions.GetFirstRevision()
		; revision+1 > fristRevision
		; --revision)
	{
		DWORD index = revisions[revision];
		if (index != -1)
		{
			WriteRevisionInfo (file, info, (DWORD)revision, index);
		}
	}

	file << footer;
}

