#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "RevisionIndex.h"
#include "RevisionInfoContainer.h"

///////////////////////////////////////////////////////////////
//
// CCachedLogInfo
//
///////////////////////////////////////////////////////////////

class CCachedLogInfo
{
private:

	// where we load / save our cached data

	std::wstring fileName;

	// revision index and the log info itself

	CRevisionIndex revisions;
	CRevisionInfoContainer logInfo;

	// revision has been added or Clear() has been called

	bool modified;

	// revision has been added (otherwise, AddChange is forbidden)

	bool revisionAdded;

	// stream IDs

	enum
	{
		REVISIONS_STREAM_ID = 1,
		LOG_INFO_STREAM_ID = 2
	};

	bool GetXMLTag ( const std::string& log
				   , size_t start
				   , size_t parentEnd
				   , const std::string& tagName
				   , size_t& tagStart
				   , size_t& tagEnd);
	std::string GetXMLTaggedText ( const std::string& log
							     , size_t start
							     , size_t end
							     , const std::string& tagName);

	size_t GetXMLAttributeOffset ( const std::string& log
							     , size_t start
								 , size_t end
					             , const std::string& attribute);
	size_t GetXMLRevisionAttribute ( const std::string& log
								   , size_t start
								   , size_t end
					               , const std::string& attribute);
	std::string GetXMLTextAttribute ( const std::string& log
								    , size_t start
								    , size_t end
						            , const std::string& attribute);

	void ParseChanges (const std::string& log, size_t current, size_t changesEnd);
	void ParseXMLLog (const std::string& log, size_t current, size_t logEnd);

public:

	// for convenience

	typedef CRevisionInfoContainer::TChangeAction TChangeAction;

	// construction / destruction (nothing to do)

	CCachedLogInfo (const std::wstring& aFileName);
	~CCachedLogInfo (void);

	// cache persistency

	void Load();
	bool IsModified();
	void Save();
	void Save (const std::wstring& newFileName);

	// XML I/O

	void LoadFromXML (const std::wstring& xmlFileName);
	void SaveAsXML (const std::wstring& xmlFileName);

	// data access

	const std::wstring& GetFileName() const;
	const CRevisionIndex& GetRevisions() const;
	const CRevisionInfoContainer& GetLogInfo() const;

	// data modification (mirrors CRevisionInfoContainer)

	void Insert ( size_t revision
				, const std::string& author
				, const std::string& comment
				, DWORD timeStamp);

	void AddChange ( TChangeAction action
				   , const std::string& path
				   , const std::string& fromPath
				   , DWORD fromRevision);

	void Clear();
};

///////////////////////////////////////////////////////////////
// cache persistency
///////////////////////////////////////////////////////////////

inline bool CCachedLogInfo::IsModified()
{
	return modified;
}

inline void CCachedLogInfo::Save()
{
	Save (fileName);
}

///////////////////////////////////////////////////////////////
// data access
///////////////////////////////////////////////////////////////

inline const std::wstring& CCachedLogInfo::GetFileName() const
{
	return fileName;
}

inline const CRevisionIndex& CCachedLogInfo::GetRevisions() const
{
	return revisions;
}

inline const CRevisionInfoContainer& CCachedLogInfo::GetLogInfo() const
{
	return logInfo;
}

///////////////////////////////////////////////////////////////
// data modification (mirrors CRevisionInfoContainer)
///////////////////////////////////////////////////////////////

inline void CCachedLogInfo::AddChange ( TChangeAction action
								       , const std::string& path
									   , const std::string& fromPath
									   , DWORD fromRevision)
{
	assert (revisionAdded);

	logInfo.AddChange (action, path, fromPath, fromRevision);
}

