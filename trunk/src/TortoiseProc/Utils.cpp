﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "resource.h"
#include "utils.h"
#include "MessageBox.h"
#include "Registry.h"
#include "TSVNPath.h"

#ifndef COMPILE_NEWAPIS_STUBS
#	define COMPILE_NEWAPIS_STUBS
#endif
#define WANT_GETLONGPATHNAME_WRAPPER
#include "NewAPIs.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

CString CUtils::GetTempFile(const CString& origfilename)
{
	TCHAR path[MAX_PATH];
	TCHAR tempF[MAX_PATH];
	::GetTempPath (MAX_PATH, path);
	CString tempfile;
	if (origfilename.IsEmpty())
	{
		::GetTempFileName (path, TEXT("svn"), 0, tempF);
		tempfile = CString(tempF);
	}
	else
	{
		int i=0;
		do
		{
			tempfile.Format(_T("%s\\svn%3.3x.tmp%s"), path, i, (LPCTSTR)CUtils::GetFileExtFromPath(origfilename));
			i++;
		} while (PathFileExists(tempfile));
	}
	return tempfile;
}

CTSVNPath CUtils::GetTempFilePath(const CTSVNPath& origfilename)
{
	return CTSVNPath(GetTempFile(origfilename.GetWinPathString()));
}
CTSVNPath CUtils::GetTempFilePath()
{
	return CTSVNPath(GetTempFile());
}


BOOL CUtils::StartExtMerge(const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
						   		const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	CString ext = mergedfile.GetFileExtension();
	CString com = regCom;

	if (ext != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + ext.MakeLower());
		if (CString(mergetool) != "")
		{
			com = mergetool;
			com = _T("\"") + com + _T("\"");
		}
	}
	
	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
		com = tortoiseMergePath;
		if (com.IsEmpty())
		{
			TCHAR tmerge[MAX_PATH];
			GetModuleFileName(NULL, tmerge, MAX_PATH);
			com = tmerge;
			com.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
		}
		com = _T("\"") + com + _T("\"");
		com = com + _T(" /base:%base /theirs:%theirs /yours:%mine /merged:%merged");
		com = com + _T(" /basename:%bname /theirsname:%tname /yoursname:%yname /mergedname:%mname");
	}
	if (basefile.IsEmpty())
	{
		com.Replace(_T("/base:%base"), _T(""));
		com.Replace(_T("%base"), _T(""));		
	}
	else
		com.Replace(_T("%base"), _T("\"") + basefile.GetWinPathString() + _T("\""));
	if (theirfile.IsEmpty())
	{
		com.Replace(_T("/theirs:%theirs"), _T(""));
		com.Replace(_T("%theirs"), _T(""));
	}
	else
		com.Replace(_T("%theirs"), _T("\"") + theirfile.GetWinPathString() + _T("\""));
	if (yourfile.IsEmpty())
	{
		com.Replace(_T("/yours:%mine"), _T(""));
		com.Replace(_T("%mine"), _T(""));
	}
	else
		com.Replace(_T("%mine"), _T("\"") + yourfile.GetWinPathString() + _T("\""));
	if (mergedfile.IsEmpty())
	{
		com.Replace(_T("/merged:%merged"), _T(""));
		com.Replace(_T("%merged"), _T(""));
	}
	else
		com.Replace(_T("%merged"), _T("\"") + mergedfile.GetWinPathString() + _T("\""));
	if (basename.IsEmpty())
	{
		com.Replace(_T("/basename:%bname"), _T(""));
		com.Replace(_T("%bname"), _T(""));
	}
	else
		com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
	if (theirname.IsEmpty())
	{
		com.Replace(_T("/theirsname:%tname"), _T(""));
		com.Replace(_T("%tname"), _T(""));
	}
	else
		com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
	if (yourname.IsEmpty())
	{
		com.Replace(_T("/yoursname:%yname"), _T(""));
		com.Replace(_T("%yname"), _T(""));
	}
	else
		com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
	if (mergedname.IsEmpty())
	{
		com.Replace(_T("/mergedname:%mname"), _T(""));
		com.Replace(_T("%mname"), _T(""));
	}
	else
		com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

	if(!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CUtils::StartDiffViewer(const CTSVNPath& file, const CTSVNPath& dir, BOOL bWait,	CString name1, CString name2, CString ext, BOOL bReversed, const CString& patchorig, const CString& patchpatched)
{
	// if "dir" is actually a file, then don't start the unified diff viewer
	// but the file diff application (e.g. TortoiseMerge, WinMerge, WinDiff, P4Diff, ...)
	CString viewer;
	if (dir.IsDirectory() || (dir.IsEmpty()))
	{
		CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
		viewer = v;
		if ((viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0)) && dir.IsEmpty())
		{
			//first try the default app which is associated with diff files
			CRegString diff = CRegString(_T(".diff\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = diff;
			viewer = viewer + _T("\\Shell\\Open\\Command\\");
			CRegString diffexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = diffexe;
			if (viewer.IsEmpty())
			{
				CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
				viewer = txt;
				viewer = viewer + _T("\\Shell\\Open\\Command\\");
				CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
				viewer = txtexe;
			} // if (viewer.IsEmpty()) 
			TCHAR buf[32*1024];
			ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
			viewer = buf;
		} // if (viewer.IsEmpty())
		if ((viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0)) && !dir.IsEmpty())
		{
			// use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			if (viewer.IsEmpty())
			{
				TCHAR tmerge[MAX_PATH];
				GetModuleFileName(NULL, tmerge, MAX_PATH);
				viewer = tmerge;
				viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
			}
			viewer = _T("\"") + viewer + _T("\"");
			viewer = viewer + _T(" /patchpath:%path /diff:%base");
			if (!patchorig.IsEmpty())
				viewer = viewer + _T(" /patchoriginal:\"") + patchorig + _T("\"");
			if (!patchpatched.IsEmpty())
				viewer = viewer + _T(" /patchpatched:\"") + patchpatched + _T("\"");
		} // if (viewer.IsEmpty() && !dir.IsEmpty())
		if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
			return FALSE;
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"), _T("\"")+file.GetWinPathString()+_T("\""));
		}
		else if (viewer.Find(_T("%1")) >= 0)
		{
			viewer.Replace(_T("%1"), _T("\"")+file.GetWinPathString()+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += _T("\"") + file.GetWinPathString() + _T("\"");
		}
		if (viewer.Find(_T("%path")) >= 0)
		{
			viewer.Replace(_T("%path"), _T("\"") + dir.GetWinPathString() + _T("\""));
		}
		if (bReversed)
			viewer += _T(" /reversedpatch");
	} // if ((PathIsDirectory(dir)) || (dir.IsEmpty())) 
	else
	{
		// not a unified diff
		CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
		viewer = diffexe;
		if (ext != "")
		{
			// is there an extension specific diff tool?
			CRegString difftool(_T("Software\\TortoiseSVN\\DiffTools\\") + ext.MakeLower());
			if (CString(difftool) != "")
			{
				viewer = difftool;
				viewer = _T("\"") + viewer + _T("\"");
			}
		}
		if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
		{
			//no registry entry for a diff program
			//use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			if (viewer.IsEmpty())
			{
				TCHAR tmerge[MAX_PATH];
				GetModuleFileName(NULL, tmerge, MAX_PATH);
				viewer = tmerge;
				viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
			}
			viewer = _T("\"") + viewer + _T("\"");
			viewer = viewer + _T(" /base:%base /yours:%mine /basename:%bname /yoursname:%yname");
		} 
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"),  _T("\"")+file.GetWinPathString()+_T("\""));
		}
		if (viewer.Find(_T("%mine")) >= 0)
		{
			viewer.Replace(_T("%mine"),  _T("\"")+dir.GetWinPathString()+_T("\""));
		}
		if (name1.IsEmpty())
		{
			name1 = file.GetWinPathString();
		}
		viewer.Replace(_T("%bname"), _T("\"") + name1 + _T("\""));
		if (name2.IsEmpty())
		{
			name2 = dir.GetWinPathString();
		}
		viewer.Replace(_T("%yname"), _T("\"") + name2 + _T("\""));
	}

	UINT idErrMessageFormat;
	if ((dir.IsDirectory()) || (dir.IsEmpty()))
		idErrMessageFormat = IDS_ERR_DIFFVIEWSTART;
	else
		idErrMessageFormat = IDS_ERR_EXTDIFFSTART;

	if(!LaunchApplication(viewer, idErrMessageFormat, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + _T("\\Shell\\Open\\Command\\");
	CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;
	TCHAR buf[32*1024];
	ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
	viewer = buf;
	ExpandEnvironmentStrings(file, buf, MAX_PATH);
	file = buf;
	file = _T("\"")+file+_T("\"");
	if (viewer.IsEmpty())
	{
		OPENFILENAME ofn;		// common dialog box structure
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		//ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		CString sFilter;
		sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy (pszFilters, sFilter);
		// Replace '|' delimeters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		} // while (ptr != pszFilters) 
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		CUtils::RemoveAccelerators(temp);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			delete [] pszFilters;
			viewer = CString(ofn.lpstrFile);
		} // if (GetOpenFileName(&ofn)==TRUE)
		else
		{
			delete [] pszFilters;
			return FALSE;
		}
	}
	if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"),  _T("\"")+file+_T("\""));
	}
	else
	{
		viewer += _T(" ");
		viewer += _T("\"")+file+_T("\"");
	}

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, false))
	{
		return FALSE;
	}
	return TRUE;
}

void CUtils::Unescape(char * psz)
{
	char * pszSource = psz;
	char * pszDest = psz;

	static const char szHex[] = "0123456789ABCDEF";

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				 *(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			char nValue = '?';
			char * pszLow = NULL;
			char * pszHigh = NULL;
			pszSource++;

			*pszSource = (char) toupper(*pszSource);
			pszHigh = strchr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (char) toupper(*pszSource);
				pszLow = strchr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (char) (((pszHigh - szHex) << 4) +
									(pszLow - szHex));
				}
			} // if (pszHigh != NULL) 
			*pszDest++ = nValue;
		} 
		else
			*pszDest++ = *pszSource;
			
		pszSource++;
	}

	*pszDest = '\0';
}

CStringA CUtils::PathEscape(const CStringA& path)
{
	CStringA ret = path;
	ret.Replace((" "), ("%20"));
	ret.Replace(("^"), ("%5E"));
	ret.Replace(("&"), ("%26"));
	ret.Replace(("`"), ("%60"));
	ret.Replace(("{"), ("%7B"));
	ret.Replace(("}"), ("%7D"));
	ret.Replace(("|"), ("%7C"));
	ret.Replace(("]"), ("%5D"));
	ret.Replace(("["), ("%5B"));
	ret.Replace(("\""), ("%22"));
	ret.Replace(("<"), ("%3C"));
	ret.Replace((">"), ("%3E"));
	ret.Replace(("#"), ("%23"));
	ret.Replace(("?"), ("%3F"));
	ret.Replace(("\\"), ("%5C"));
	ret.Replace(("file:///%5C"), ("file:///\\"));
	ret.Replace(("file:////%5C"), ("file:////\\"));

	return ret;
}

BOOL CUtils::IsEscaped(const CStringA& path)
{
	if (path.Find("%20")>=0)
		return TRUE;
	if (path.Find("%5E")>=0)
		return TRUE;
	if (path.Find("%26")>=0)
		return TRUE;
	if (path.Find("%60")>=0)
		return TRUE;
	if (path.Find("%7B")>=0)
		return TRUE;
	if (path.Find("%7D")>=0)
		return TRUE;
	if (path.Find("%7C")>=0)
		return TRUE;
	if (path.Find("%5D")>=0)
		return TRUE;
	if (path.Find("%5B")>=0)
		return TRUE;
	if (path.Find("%22")>=0)
		return TRUE;
	if (path.Find("%3C")>=0)
		return TRUE;
	if (path.Find("%3E")>=0)
		return TRUE;
	if (path.Find("%5C")>=0)
		return TRUE;
	if (path.Find("%23")>=0)
		return TRUE;
	if (path.Find("%3F")>=0)
		return TRUE;
	return FALSE;
}

CString CUtils::GetVersionFromFile(const CString & p_strDateiname)
{
	struct TRANSARRAY
	{
		WORD wLanguageID;
		WORD wCharacterSet;
	};

	CString strReturn;
	DWORD dwReserved,dwBufferSize;
	dwBufferSize = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)p_strDateiname,&dwReserved);

	if (dwBufferSize > 0)
	{
		LPVOID pBuffer = (void*) malloc(dwBufferSize);

		if (pBuffer != (void*) NULL)
		{
			UINT        nInfoSize = 0,
			nFixedLength = 0;
			LPSTR       lpVersion = NULL;
			VOID*       lpFixedPointer;
			TRANSARRAY* lpTransArray;
			CString     strLangProduktVersion;

			GetFileVersionInfo((LPTSTR)(LPCTSTR)p_strDateiname,
			dwReserved,
			dwBufferSize,
			pBuffer);

			// Abfragen der aktuellen Sprache
			VerQueryValue(	pBuffer,
							_T("\\VarFileInfo\\Translation"),
							&lpFixedPointer,
							&nFixedLength);
			lpTransArray = (TRANSARRAY*) lpFixedPointer;

			strLangProduktVersion.Format(_T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
			lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

			VerQueryValue(pBuffer,
			(LPTSTR)(LPCTSTR)strLangProduktVersion,
			(LPVOID *)&lpVersion,
			&nInfoSize);
			strReturn = (LPCTSTR)lpVersion;
			free(pBuffer);
		}
	} 

	return strReturn;
}

CString CUtils::GetFileNameFromPath(CString sPath)
{
	CString ret;
	sPath.Replace(_T("/"), _T("\\"));
	ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
	return ret;
}

CString CUtils::GetFileExtFromPath(const CString& sPath)
{
	int dotPos = sPath.ReverseFind('.');
	int slashPos = sPath.ReverseFind('/');
	if (dotPos > slashPos)
		return sPath.Mid(dotPos);
	return CString();
}

CString CUtils::WritePathsToTempFile(CString paths)
{
	CString tempfile = CUtils::GetTempFile();
	try
	{
		CStdioFile file(tempfile, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		int pos = -1;
		CString temp;
		do
		{
			pos = paths.Find('*');
			if (pos>=0)
				temp = paths.Left(pos);
			else
				temp = paths;
			temp = CUtils::GetLongPathname(temp);
			file.WriteString(temp + _T("\n"));
			paths = paths.Mid(pos+1);
		} while (pos >= 0);
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE(_T("CFileException in Commit!\n"));
		pE->Delete();
	}
	return tempfile;
}

CString CUtils::GetLongPathname(const CString& path)
{
	if (path.IsEmpty())
		return path;
	TCHAR pathbuf[MAX_PATH];
	TCHAR pathbufcanonicalized[MAX_PATH];
	DWORD ret = 0;
	if (PathCanonicalize(pathbufcanonicalized, path))
	{
		ret = ::GetLongPathName(pathbufcanonicalized, pathbuf, MAX_PATH);
	}
	else
	{
		ret = ::GetLongPathName(path, pathbuf, MAX_PATH);
	}
	if ((ret == 0)||(ret > MAX_PATH))
		return path;
	return CString(pathbuf, ret);
}

BOOL CUtils::FileCopy(CString srcPath, CString destPath, BOOL force)
{
	srcPath.Replace('/', '\\');
	destPath.Replace('/', '\\');
	// now make sure that the destination directory exists
	int ind = 0;
	while (destPath.Find('\\', ind)>=0)
	{
		if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
		{
			if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), NULL))
				return FALSE;
		}
		ind = destPath.Find('\\', ind)+1;
	}
	if (PathIsDirectory(srcPath))
	{
		if (!PathIsDirectory(destPath))
			return CreateDirectory(destPath, NULL);
		return TRUE;
	}
	return (CopyFile(srcPath, destPath, !force));
}

BOOL CUtils::CheckForEmptyDiff(const CTSVNPath& sDiffPath)
{
	DWORD length = 0;
	HANDLE hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return TRUE;
	length = ::GetFileSize(hFile, NULL);
	::CloseHandle(hFile);
	if (length < 4)
		return TRUE;
	return FALSE;

}

void CUtils::RemoveAccelerators(CString& text)
{
	int pos = 0;
	while ((pos=text.Find('&',pos))>=0)
	{
		if (text.GetLength() > (pos-1))
		{
			if (text.GetAt(pos+1)!=' ')
				text.Delete(pos);
		}
		pos++;
	}
}


bool CUtils::WriteAsciiStringToClipboard(const CStringA& sClipdata, HWND hOwningWnd)
{
	//TODO The error handling in here is not exactly sparkling!

	if (OpenClipboard(hOwningWnd))
	{
		EmptyClipboard();
		HGLOBAL hClipboardData;
		hClipboardData = GlobalAlloc(GMEM_DDESHARE, sClipdata.GetLength()+1);
		char * pchData;
		pchData = (char*)GlobalLock(hClipboardData);
		strcpy(pchData, (LPCSTR)sClipdata);
		GlobalUnlock(hClipboardData);
		SetClipboardData(CF_TEXT,hClipboardData);
		CloseClipboard();

		return true;
	} // if (OpenClipboard()) 

	return false;
}

void CUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(NULL);
	logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(NULL, hScreenDC);
	logFont.lfWidth          = 0;
	logFont.lfEscapement     = 0;
	logFont.lfOrientation    = 0;
	logFont.lfWeight         = FW_NORMAL;
	logFont.lfItalic         = 0;
	logFont.lfUnderline      = 0;
	logFont.lfStrikeOut      = 0;
	logFont.lfCharSet        = DEFAULT_CHARSET;
	logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality        = DRAFT_QUALITY;
	logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(logFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	//remove possible double quotes
	//(depends on if the user added them in the settings him/herself or not)
	CString cleanCommandLine(sCommandLine);
	while (cleanCommandLine.Replace(_T("\"\""), _T("\"")) > 0)
	{
		;
	}
	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		if(idErrMessageFormat != 0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			CString temp;
			temp.Format(idErrMessageFormat, lpMsgBuf);
			CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
		}
		return false;
	} // if (CreateProcess(NULL /*(LPCTSTR)cleanCommandLine*/, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	return true;
}

/**
* Launch the external blame viewer
*/
bool CUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& sLogFile, const CString& sOriginalFile)
{
	TCHAR tblame[MAX_PATH];
	GetModuleFileName(NULL, tblame, MAX_PATH);

	CString viewer = tblame;
	viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseBlame.exe"));
	viewer += _T(" \"") + sBlameFile + _T("\"");
	viewer += _T(" \"") + sLogFile + _T("\"");
	viewer += _T(" \"") + sOriginalFile + _T("\"");
	
	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
}