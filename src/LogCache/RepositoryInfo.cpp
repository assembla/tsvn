// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "StdAfx.h"
#include "RepositoryInfo.h"

#include "svn_client.h"

#include "SVN.h"
#include "TSVNPath.h"

// begin namespace LogCache

namespace LogCache
{

// construct the dump file name

CString CRepositoryInfo::GetFileName() const
{
    return cacheFolder + _T("\\Repositories.dat");
}

// read the dump file

void CRepositoryInfo::Load()
{
    CFile file (GetFileName(), CFile::modeRead);
    CArchive stream (&file, CArchive::load);

    int count = 0;
    stream >> count;

    for (int i = 0; i < count; ++i)
    {
        SPerRepositoryInfo info;
        stream >> info.root 
               >> info.uuid
               >> info.headURL
               >> info.headRevision
               >> info.headLookupTime;

        data[info.root] = info;
    }
}

// find cache entry (or data::end())

CRepositoryInfo::TData::const_iterator 
CRepositoryInfo::Lookup (const CTSVNPath& url)
{
    CString fullURL (url.GetSVNPathString());

    // the repository root URL should be a prefix of url

    TData::const_iterator iter = data.lower_bound (fullURL);

    // no suitable prefix?

    if (iter == data.end())
        return iter;

    // does prefix match?

    if (iter->first == fullURL.Left (iter->first.GetLength()))
        return iter;

    // not found

    return data.end();
}

// construction / destruction: auto-load and save

CRepositoryInfo::CRepositoryInfo (const CString& cacheFolderPath)
    : cacheFolder (cacheFolderPath)
{
    Load();
}

CRepositoryInfo::~CRepositoryInfo(void)
{
    Flush();
}

// look-up and ask SVN if the info is not in cache. 
// cache the result.

CString CRepositoryInfo::GetRepositoryRoot (const CTSVNPath& url)
{
    CString uuid;
    return GetRepositoryRootAndUUID (url, uuid);
}

CString CRepositoryInfo::GetRepositoryRootAndUUID ( const CTSVNPath& url
                                                  , CString& sUUID)
{
    TData::const_iterator iter = Lookup (url);
    if (iter == data.end())
    {
        SPerRepositoryInfo info;
        info.root = SVN().GetRepositoryRootAndUUID (url, info.uuid);
        info.headRevision = NO_REVISION;
        info.headLookupTime = -1;

        data [info.root] = info;
        return info.root;
    }

    return iter->first;
}

// find the root folder to a given UUID (e.g. URL for given cache file).
// Returns an empty string, if no suitable entry has been found.

CString CRepositoryInfo::GetRootFromUUID (const CString& sUUID)
{
    // scan all data

    for ( TData::const_iterator iter = data.begin(), end = data.end()
        ; iter != end
        ; ++iter)
    {
        if (iter->second.uuid == sUUID)
            return iter->first;
    }

    // not found

    return CString();
}

// write all changes to disk

void CRepositoryInfo::Flush()
{
    CFile file (GetFileName(), CFile::modeWrite | CFile::modeCreate);
    CArchive stream (&file, CArchive::store);

    stream << static_cast<int>(data.size()) << L'\t';

    for ( TData::const_iterator iter = data.begin(), end = data.end()
        ; iter != end
        ; ++iter)
    {
        stream << iter->second.root 
               << iter->second.uuid 
               << iter->second.headURL 
               << iter->second.headRevision 
               << iter->second.headLookupTime;
    }
}

// clear cache

void CRepositoryInfo::Clear()
{
    data.clear();
}

// end namespace LogCache

}