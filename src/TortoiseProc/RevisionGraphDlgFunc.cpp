// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MemDC.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\revisiongraphwnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

void CRevisionGraphWnd::InitView()
{
	m_bIsRubberBand = false;
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	m_arVertPositions.RemoveAll();
	m_targetsbottom.clear();
	m_targetsright.clear();
	m_GraphRect.SetRectEmpty();
	m_ViewRect.SetRectEmpty();
	GetViewSize();
	BuildConnections();
	SetScrollbars(0,0,m_ViewRect.Width(),m_ViewRect.Height());
}

void CRevisionGraphWnd::BuildPreview()
{
	m_Preview.DeleteObject();
	if (!m_bShowOverview)
		return;

	float origZoom = m_fZoomFactor;
	// zoom the graph so that it is completely visible in the window
	DoZoom(1.0);
	GetViewSize();
	float horzfact = float(m_GraphRect.Width())/float(REVGRAPH_PREVIEW_WIDTH);
	float vertfact = float(m_GraphRect.Height())/float(REVGRAPH_PREVIEW_HEIGTH);
	float fZoom = 1.0f/(max(horzfact, vertfact));
	if (fZoom > 1.0f)
		fZoom = 1.0f;
	int trycounter = 0;
	m_fZoomFactor = fZoom;
	while ((trycounter < 5)&&((m_GraphRect.Width()>REVGRAPH_PREVIEW_WIDTH)||(m_GraphRect.Height()>REVGRAPH_PREVIEW_HEIGTH)))
	{
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
		GetViewSize();
		fZoom *= 0.95f;
		trycounter++;
	}

	CClientDC ddc(this);
	CDC dc;
	if (!dc.CreateCompatibleDC(&ddc))
		return;
	m_Preview.CreateCompatibleBitmap(&ddc, REVGRAPH_PREVIEW_WIDTH, REVGRAPH_PREVIEW_HEIGTH);
	HBITMAP oldbm = (HBITMAP)dc.SelectObject(m_Preview);
	// paint the whole graph
	DrawGraph(&dc, m_ViewRect, 0, 0, true);
	// now we have a bitmap the size of the preview window
	dc.SelectObject(oldbm);
	dc.DeleteDC();

	DoZoom(origZoom);
}

void CRevisionGraphWnd::SetScrollbars(int nVert, int nHorz, int oldwidth, int oldheight)
{
	CRect clientrect;
	GetClientRect(&clientrect);
	CRect * pRect = GetGraphSize();
	SCROLLINFO ScrollInfo;
	ScrollInfo.cbSize = sizeof(SCROLLINFO);
	ScrollInfo.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &ScrollInfo);
	if ((nVert)||(oldheight==0))
		ScrollInfo.nPos = nVert;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Height() / oldheight;
	ScrollInfo.fMask = SIF_ALL;
	ScrollInfo.nMin = 0;
	ScrollInfo.nMax = pRect->bottom;
	ScrollInfo.nPage = clientrect.Height();
	ScrollInfo.nTrackPos = 0;
	SetScrollInfo(SB_VERT, &ScrollInfo);
	GetScrollInfo(SB_HORZ, &ScrollInfo);
	if ((nHorz)||(oldwidth==0))
		ScrollInfo.nPos = nHorz;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Width() / oldwidth;
	ScrollInfo.nMax = pRect->right;
	ScrollInfo.nPage = clientrect.Width();
	SetScrollInfo(SB_HORZ, &ScrollInfo);
}

void CRevisionGraphWnd::ClearEntryConnections()
{
	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * reventry = m_entryPtrs[i];
		reventry->bottomconnections = 0;
		reventry->leftconnections = 0;
		reventry->rightconnections = 0;
		reventry->bottomlines = 0;
		reventry->rightlines = 0;
	}
	m_targetsright.clear();
	m_targetsbottom.clear();
}

void CRevisionGraphWnd::BuildConnections()
{
	// create an array which holds the vertical position of each
	// revision entry. Since there can be several entries in the
	// same revision, this speeds up the search for the right
	// position for drawing.
	m_arVertPositions.RemoveAll();
	svn_revnum_t vprev = 0;
	int currentvpos = 0;
	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * reventry = m_entryPtrs[i];
		if (reventry->revision != vprev)
		{
			vprev = reventry->revision;
			currentvpos++;
		}
		m_arVertPositions.Add(currentvpos-1);
	}
	// delete all entries which we might have left
	// in the array and free the memory they use.
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	
	ClearEntryConnections();
	
	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * reventry = m_entryPtrs[i];
		reventry->bottomconnectionsleft = reventry->bottomconnections;
		reventry->leftconnectionsleft = reventry->leftconnections;
		reventry->rightconnectionsleft = reventry->rightconnections;
		reventry->bottomlinesleft = reventry->bottomlines;
		reventry->rightlinesleft = reventry->rightlines;
	}

	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * reventry = m_entryPtrs[i];
		float vertpos = (float)m_arVertPositions[i];

		for (size_t j = 0, count = reventry->copyTargets.size(); j < count; ++j)
		{
			CRevisionEntry * reventry2 = reventry->copyTargets[j];
			
			// we always draw from bottom to top!			
			CPoint * pt = new CPoint[5];
			if (reventry->level < reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					//       5
					//       |
					//    3--4
					//    |
					// 1--2
					
					// x-offset for line 2-3
					int xoffset = int(float(reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/float(reventry->rightlines+1));
					// y-offset for line 3-4
					int yoffset = int(float(reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/float(reventry2->bottomlines+1));
					
					//Starting point: 1
					pt[0].y = long(vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top +
						(float(reventry->rightconnectionsleft)*(m_node_rect_heigth)/float(reventry->rightconnections+1)));	// top of rect
					reventry->rightconnectionsleft--;
					pt[0].x = long(float(reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = long((float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom)) + m_node_rect_heigth + m_node_space_top);
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = long(float(reventry2->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2.0f);
					//line up to target rect: 5
					pt[4].y = long((float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth) + m_node_space_top);
					pt[4].x = pt[3].x;
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else if (reventry->level > reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					// 5
					// |
					// 4----3
					//      |
					//      |
					//      2-----1

					// x-offset for line 2-3
					int xoffset = int(float(reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/float(reventry->rightlines+1));
					// y-offset for line 3-4
					int yoffset = int(float(reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/float(reventry2->bottomlines+1));

					//Starting point: 1
					pt[0].y = long((vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top) + 
						float(reventry->leftconnectionsleft)*(m_node_rect_heigth)/float(reventry->leftconnections+1));
					reventry->leftconnectionsleft--;
					pt[0].x = long(float(reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x - xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = long((float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top + m_node_space_bottom));
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = long(float(reventry2->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += long(m_node_space_left + m_node_rect_width/2.0f);
					//line up to target rect: 5
					pt[4].x = pt[3].x;
					pt[4].y = long(float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top);
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else
			{
				// same level!
				// check first if there are other nodes in between the two connected ones
				BOOL nodesinbetween = FALSE;
				if (nodesinbetween)
				{
					// 4----3
					//      |
					//      |
					//      |
					// 1----2

					// x-offset for line 2-3
					int xoffset = int(float(reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/float(reventry->rightlines+1));
					// y-offset for line 3-4
					int yoffset = int(float(reventry2->rightconnectionsleft)*(m_node_rect_heigth)/float(reventry2->rightconnections+1));
					reventry2->rightconnectionsleft--;
										
					//Starting point: 1
					pt[0].y = long((vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top) + 
						float(reventry->rightconnectionsleft)*(m_node_rect_heigth)/float(reventry->rightconnections+1));
					reventry->rightconnectionsleft--;
					pt[0].x = long((float(reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = long(float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += yoffset;
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = long((float(reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width);
					pt[4].y = pt[3].y;
					pt[4].x = pt[3].x;
				}
				else
				{
					if (reventry->revision < reventry2->revision)
					{
						// 2
						// |
						// |
						// 1
						pt[0].y = long(vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
						pt[0].x = long((float(reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2.0f));
						pt[1].y = pt[0].y;
						pt[1].x = pt[0].x;
						pt[2].y = long(float(m_arVertPositions[reventry2->index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top + m_node_rect_heigth);
						pt[2].x = pt[0].x;
						pt[3].y = pt[2].y;
						pt[3].x = pt[2].x;
						pt[4].y = pt[3].y;
						pt[4].x = pt[3].x;
					}
					else
					{
						ATLASSERT(false);
					}
				}
			}
			INT_PTR conindex = m_arConnections.Add(pt);

			// we add the connection index to each revision entry which the connection
			// passes by vertically. We use this to reduce the time to draw the connections,
			// because we know which nodes are in the visible area but not which connections.
			// By doing this, we can simply get the connections we have to draw from
			// the nodes we draw.
/*			for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(reventry->revision); it != m_mapEntryPtrs.upper_bound(reventry2->revision); ++it)
			{
				it->second->connections.insert(conindex);
			}
			DecrementSpaceLines(sentry);*/
		}
	}
}

CRect * CRevisionGraphWnd::GetGraphSize()
{
	if (m_GraphRect.Height() != 0)
		return &m_GraphRect;
	m_GraphRect.top = 0;
	m_GraphRect.left = 0;
	int lastrev = -1;
	if ((m_maxlevel == 0) || (m_numRevisions == 0) || (m_maxurllength == 0) || m_maxurl.IsEmpty())
	{
		for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
		{
			CRevisionEntry * reventry = m_entryPtrs[i];
			if (m_maxlevel < reventry->level)
				m_maxlevel = reventry->level;
			if (lastrev != reventry->revision)
			{
				m_numRevisions++;
				lastrev = reventry->revision;
			}
			size_t len = reventry->path.GetPath().size();
			if (m_maxurllength < len)
			{
				m_maxurllength = len;
				m_maxurl = CUnicodeUtils::GetUnicode (reventry->path.GetPath().c_str());
			}
		}
	}

	// calculate the width of the nodes by looking
	// at the url lengths
	CRect r;
	CDC * pDC = this->GetDC();
	if (pDC)
	{
		CFont * pOldFont = pDC->SelectObject(GetFont(TRUE));
		pDC->DrawText(m_maxurl, &r, DT_CALCRECT);
		// keep the width inside reasonable values.
		m_node_rect_width = min(500.0f * m_fZoomFactor, r.Width()+40.0f);
		m_node_rect_width = max(NODE_RECT_WIDTH * m_fZoomFactor, m_node_rect_width);
		pDC->SelectObject(pOldFont);
	}
	ReleaseDC(pDC);

	m_GraphRect.right = long(float(m_maxlevel) * (m_node_rect_width + m_node_space_left + m_node_space_right));
	m_GraphRect.bottom = long(float(m_numRevisions) * (m_node_rect_heigth + m_node_space_top + m_node_space_bottom));
	return &m_GraphRect;
}

CRect * CRevisionGraphWnd::GetViewSize()
{
	if (m_ViewRect.Height() != 0)
		return &m_ViewRect;
	m_ViewRect = GetGraphSize();
	CRect rect;
	GetClientRect(&rect);
	if (m_ViewRect.Width() < rect.Width())
	{
		m_ViewRect.left = rect.left;
		m_ViewRect.right = rect.right;
	}
	if (m_ViewRect.Height() < rect.Height())
	{
		m_ViewRect.top = rect.top;
		m_ViewRect.bottom = rect.bottom;
	}
	return &m_ViewRect;
}

int CRevisionGraphWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

void CRevisionGraphWnd::CompareRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));

	SVNRev peg = (SVNRev)(bHead ? m_SelectedEntry1->revision : SVNRev());

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowCompare(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
		url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
		peg);
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowUnifiedDiff(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
						 url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
						 m_SelectedEntry1->revision);
}

CTSVNPath CRevisionGraphWnd::DoUnifiedDiff(bool bHead, CString& sRoot, bool& bIsFolder)
{
	theApp.DoWaitCursor(1);
	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(_T("test.diff")));
	// find selected objects
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);
	
	// find out if m_sPath points to a file or a folder
	if (SVN::PathIsURL(m_sPath))
	{
		SVNInfo info;
		const SVNInfoData * infodata = info.GetFirstFileInfo(CTSVNPath(m_sPath), SVNRev::REV_HEAD, SVNRev::REV_HEAD);
		if (infodata)
		{
			bIsFolder = (infodata->kind == svn_node_dir);
		}
	}
	else
	{
		bIsFolder = CTSVNPath(m_sPath).IsDirectory();
	}
	
	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));
	CTSVNPath url1_temp = url1;
	CTSVNPath url2_temp = url2;
	INT_PTR iMax = min(url1_temp.GetSVNPathString().GetLength(), url2_temp.GetSVNPathString().GetLength());
	INT_PTR i = 0;
	for ( ; ((i<iMax) && (url1_temp.GetSVNPathString().GetAt(i)==url2_temp.GetSVNPathString().GetAt(i))); ++i)
		;
	while (url1_temp.GetSVNPathString().GetLength()>i)
		url1_temp = url1_temp.GetContainingDirectory();

	if (bIsFolder)
		sRoot = url1_temp.GetSVNPathString();
	else
		sRoot = url1_temp.GetContainingDirectory().GetSVNPathString();
	
	if (url1.IsEquivalentTo(url2))
	{
		if (!svn.PegDiff(url1, SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			svn_depth_infinity, TRUE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	else
	{
		if (!svn.Diff(url1, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			url2, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			svn_depth_infinity, TRUE, FALSE, FALSE, CString(), false, tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);		
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	theApp.DoWaitCursor(-1);
	return tempfile;
}

void CRevisionGraphWnd::DoZoom(float fZoomFactor)
{
	m_fZoomFactor = fZoomFactor;
	m_node_space_left = NODE_SPACE_LEFT * fZoomFactor;
	m_node_space_right = NODE_SPACE_RIGHT * fZoomFactor;
	m_node_space_line = NODE_SPACE_LINE * fZoomFactor;
	m_node_rect_heigth = NODE_RECT_HEIGTH * fZoomFactor;
	m_node_space_top = NODE_SPACE_TOP * fZoomFactor;
	m_node_space_bottom = NODE_SPACE_BOTTOM * fZoomFactor;
	m_nFontSize = int(12.0f * fZoomFactor);
	m_RoundRectPt.x = int(ROUND_RECT * fZoomFactor);
	m_RoundRectPt.y = int(ROUND_RECT * fZoomFactor);
	m_nIconSize = int(32 * fZoomFactor);
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	InitView();
	Invalidate();
}












