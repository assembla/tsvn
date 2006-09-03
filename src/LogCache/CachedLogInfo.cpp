#include "StdAfx.h"
#include ".\cachedloginfo.h"

#include "RootInStream.h"
#include "RootOutStream.h"

// construction / destruction (nothing to do)

CCachedLogInfo::CCachedLogInfo (const std::wstring& aFileName)
	: fileName (aFileName)
	, modified (false)
	, revisionAdded (false)
{
}

CCachedLogInfo::~CCachedLogInfo (void)
{
}

// cache persistency

void CCachedLogInfo::Load()
{
	assert (revisions.GetLastRevision() == 0);

	CRootInStream stream (fileName);

	// write the data

	IHierarchicalInStream* revisionsStream
		= stream.GetSubStream (REVISIONS_STREAM_ID);
	*revisionsStream >> revisions;

	IHierarchicalInStream* logInfoStream
		= stream.GetSubStream (LOG_INFO_STREAM_ID);
	*logInfoStream >> logInfo;
}

void CCachedLogInfo::Save (const std::wstring& newFileName)
{
	CRootOutStream stream (newFileName);

	// write the data

	IHierarchicalOutStream* revisionsStream
		= stream.OpenSubStream (REVISIONS_STREAM_ID, COMPOSITE_STREAM_TYPE_ID);
	*revisionsStream << revisions;

	IHierarchicalOutStream* logInfoStream
		= stream.OpenSubStream (LOG_INFO_STREAM_ID, COMPOSITE_STREAM_TYPE_ID);
	*logInfoStream << logInfo;

	// all fine -> connect to the new file name

	fileName = newFileName;
}

// XML I/O

bool CCachedLogInfo::GetXMLTag ( const std::string& log
							   , size_t start
							   , size_t parentEnd
							   , const std::string& tagName
							   , size_t& tagStart
							   , size_t& tagEnd)
{
	// check our internal logic

	assert (start < parentEnd);
	assert (start != std::string::npos);
	assert (parentEnd != std::string::npos);

	// find the next tag with the given name

	tagStart = log.find ('<' + tagName, start);
	if ((tagStart == std::string::npos) || (tagStart >= parentEnd))
		return false;

	// tag has been found. return first attribute or content position

	tagStart += 1 + tagName.length();

	// find end tag

	tagEnd = log.find ("</" + tagName + '>', tagStart);
	if ((tagEnd == std::string::npos) || (tagEnd >= parentEnd))
		return false;

	// both tags have been found

	return true;
}

size_t CCachedLogInfo::GetXMLAttributeOffset ( const std::string& log
										     , size_t start
											 , size_t end
								             , const std::string& attribute)
{
	size_t tagEnd = log.find ('>', start);
	if (tagEnd < end)
		end = tagEnd;

	size_t attributeNameLength = attribute.length();
	while (start < end)
	{
		start = log.find ('=', start);
		if (start < end)
		{
			assert (start > attributeNameLength);
			std::string currentAttribute
				= log.substr (start - attributeNameLength, attributeNameLength);

			start += 2;

			if (currentAttribute == attribute)
				return start;

			start = log.find ('"', start);
		}
	}

	return std::string::npos;
}

size_t CCachedLogInfo::GetXMLRevisionAttribute ( const std::string& log
											   , size_t start
											   , size_t end
								               , const std::string& attribute)
{
	start = GetXMLAttributeOffset (log, start, end, attribute);
	return start == std::string::npos
		? -1
		: atoi (log.c_str() + start);
}

std::string CCachedLogInfo::GetXMLTextAttribute ( const std::string& log
											    , size_t start
											    , size_t end
									            , const std::string& attribute)
{
	start = GetXMLAttributeOffset (log, start, end, attribute);
	return start == std::string::npos
		? std::string()
		: log.substr (start, min (end, log.find ('"', start)) - start);
}

std::string 
CCachedLogInfo::GetXMLTaggedText ( const std::string& log
							     , size_t start
							     , size_t end
							     , const std::string& tagName)
{
	if (GetXMLTag (log, start, end, tagName, start, end))
	{
		start = log.find ('>', start)+1;
		return log.substr (start, end - start);
	}

	return std::string();
}

void CCachedLogInfo::ParseChanges ( const std::string& log
								  , size_t current
								  , size_t changesEnd)
{
	size_t changeEnd = std::string::npos;
	while (GetXMLTag (log, current, changesEnd, "path", current, changeEnd))
	{
		std::string actionText
			= GetXMLTextAttribute (log, current, changesEnd, "action");
		std::string fromPath
			= GetXMLTextAttribute (log, current, changesEnd, "copyfrom-path");
		size_t fromRevision
			= GetXMLRevisionAttribute (log, current, changesEnd, "copyfrom-rev");

		current = log.find ('>', current)+1;
		std::string path = log.substr (current, changeEnd - current);

		TChangeAction action = CRevisionInfoContainer::ACTION_ADDED;

		switch (actionText[0])
		{
		case 'A': 
			break;

		case 'M': 
			action = CRevisionInfoContainer::ACTION_CHANGED;
			break;

		case 'R': 
			action = CRevisionInfoContainer::ACTION_REPLACED;
			break;

		case 'D': 
			action = CRevisionInfoContainer::ACTION_DELETED;
			break;

		default:

			throw std::exception ("unknown action type");
		}

		AddChange (action, path, fromPath, (DWORD)fromRevision);
	}
}

void CCachedLogInfo::ParseXMLLog ( const std::string& log
								 , size_t current
								 , size_t logEnd)
{
	size_t revisionEnd = std::string::npos;
	while (GetXMLTag (log, current, logEnd, "logentry", current, revisionEnd))
	{
		size_t revision 
			= GetXMLRevisionAttribute (log, current, revisionEnd, "revision");
		std::string author
			= GetXMLTaggedText (log, current, revisionEnd, "author");
		std::string date
			= GetXMLTaggedText (log, current, revisionEnd, "date");
		std::string comment
			= GetXMLTaggedText (log, current, revisionEnd, "msg");

		tm time = {0,0,0, 0,0,0, 0,0,0};
		sscanf ( date.c_str()
			   , "%04d-%02d-%02dT%02d:%02d:%02d."
			   , &time.tm_year
			   , &time.tm_mon
			   , &time.tm_mday
			   , &time.tm_hour
			   , &time.tm_min
			   , &time.tm_sec);
		time.tm_isdst = 0;
		time.tm_year -= 1900;
		time.tm_mon -= 1;

		DWORD timeStamp = (DWORD)mktime (&time);

		Insert (revision, author, comment, timeStamp);

		size_t pathsEnd = std::string::npos;
		if (GetXMLTag (log, current, revisionEnd, "paths", current, pathsEnd))
		{
			ParseChanges (log, current, pathsEnd);
		}

		current = revisionEnd;
	}
}

void CCachedLogInfo::LoadFromXML (const std::wstring& xmlFileName)
{
	CMappedInFile file (xmlFileName);
	std::string xmlLog (file.GetBuffer(), file.GetBuffer() + file.GetSize());

	size_t logStart = std::string::npos;
	size_t logEnd = std::string::npos;

	if (GetXMLTag (xmlLog, 0, xmlLog.length(), "log", logStart, logEnd))
	{
		ParseXMLLog (xmlLog, logStart, logEnd);
	}
	else
	{
		throw std::exception ("XML file contains no log information");
	}
}

void CCachedLogInfo::SaveAsXML (const std::wstring& xmlFileName)
{
}

// data modification (mirrors CRevisionInfoContainer)

void CCachedLogInfo::Insert ( size_t revision
							 , const std::string& author
							 , const std::string& comment
							 , DWORD timeStamp)
{
	// there will be a modification

	modified = true;

	// add entry to cache and update the revision index

	DWORD index = (DWORD)logInfo.Insert (author, comment, timeStamp);
	revisions.SetRevisionIndex (revision, index);

	// you may call AddChange() now

	revisionAdded = true;
}

void CCachedLogInfo::Clear()
{
	modified = revisions.GetLastRevision() != 0;
	revisionAdded = false;

	revisions.Clear();
	logInfo.Clear();
}
