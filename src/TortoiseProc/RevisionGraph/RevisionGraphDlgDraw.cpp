// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "RevisionGraph/IRevisionGraphLayout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

/************************************************************************/
/* Graphing functions                                                   */
/************************************************************************/
CFont* CRevisionGraphWnd::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) FALSE;
		CDC * pDC = GetDC();
		m_lfBaseFont.lfHeight = -MulDiv(m_nFontSize, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
		ReleaseDC(pDC);
		// use the empty font name, so GDI takes the first font which matches
		// the specs. Maybe this will help render chinese/japanese chars correctly.
		_tcsncpy_s(m_lfBaseFont.lfFaceName, 32, _T("MS Shell Dlg 2"), 32);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CWnd::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

BOOL CRevisionGraphWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CRevisionGraphWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(&rect);
	if (m_bThreadRunning)
	{
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		CWnd::OnPaint();
		return;
	}
    else if (m_layout.get() == NULL)
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,sNoGraphText,NULL);
		return;
	}
	GetViewSize();
	DrawGraph(&dc, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
}

void CRevisionGraphWnd::DrawOctangle(CDC * pDC, const CRect& rect)
{
	int cutLen = rect.Height() / 4;
	CPoint point1(rect.left, rect.top + cutLen);
	CPoint point2(rect.left + cutLen, rect.top);
	CPoint point3(rect.right - cutLen, rect.top);
	CPoint point4(rect.right, rect.top + cutLen);
	CPoint point5(rect.right, rect.bottom - cutLen);
	CPoint point6(rect.right - cutLen, rect.bottom);
	CPoint point7(rect.left + cutLen, rect.bottom);
	CPoint point8(rect.left, rect.bottom - cutLen);
	CPoint arrPoints[8] = {
							point1,
							point2,
							point3,
							point4,
							point5,
							point6,
							point7,
							point8};

		pDC->Polygon(arrPoints, 8);
}

void CRevisionGraphWnd::DrawNode(CDC * pDC, const CRect& rect,
                                 COLORREF contour, const CVisibleGraphNode *node, NodeShape shape, 
								 HICON hIcon, int penStyle /*= PS_SOLID*/)
{
#define minmax(x, y, z) (x > 0 ? min<long>(y, z) : max<long>(y, z))
	CPen* pOldPen = 0L;
	CBrush* pOldBrush = 0L;
	CPen pen, pen2;
	CBrush brush;
	COLORREF background = GetSysColor(COLOR_WINDOW);
	COLORREF textcolor = GetSysColor(COLOR_WINDOWTEXT);

    // special case: line deleted but deletion node removed

    if (   (node->GetNext() == NULL) 
        && (node->GetClassification().Is (CNodeClassification::IS_DELETED)))
    {
        contour = m_Colors.GetColor(CColors::DeletedNode);
    }

    bool nodeSelected = (m_SelectedEntry1 == node) || (m_SelectedEntry2 == node);

	COLORREF selcolor = contour;
	int rval = (GetRValue(background)-GetRValue(contour))/2;
	int gval = (GetGValue(background)-GetGValue(contour))/2;
	int bval = (GetBValue(background)-GetBValue(contour))/2;
	selcolor = RGB(minmax(rval, GetRValue(contour)+rval, GetRValue(background)),
		minmax(gval, GetGValue(contour)+gval, GetGValue(background)),
		minmax(bval, GetBValue(contour)+bval, GetBValue(background)));

	COLORREF shadowc = RGB(abs(GetRValue(background)-GetRValue(textcolor))/2,
							abs(GetGValue(background)-GetGValue(textcolor))/2,
							abs(GetBValue(background)-GetBValue(textcolor))/2);

	TRY
	{
		// Prepare the shadow
		if (rect.Height() > 10)
		{
			CRect shadow = rect;
			CPoint shadowoffset = SHADOW_OFFSET_PT;
			if (rect.Height() < 40)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			if (rect.Height() < 30)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			if (rect.Height() < 20)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			shadow.OffsetRect(shadowoffset);

			brush.CreateSolidBrush(shadowc);
			pOldBrush = pDC->SelectObject(&brush);
			pen.CreatePen(penStyle, 1, shadowc);
			pOldPen = pDC->SelectObject(&pen);

			// Draw the shadow
			switch( shape )
			{
			case TSVNRectangle:
				pDC->Rectangle(shadow);
				break;
			case TSVNRoundRect:
				pDC->RoundRect(shadow, m_RoundRectPt);
				break;
			case TSVNOctangle:
				DrawOctangle(pDC, shadow);
				break;
			case TSVNEllipse:
				pDC->Ellipse(shadow);
				break;
			default:
				ASSERT(FALSE);	//unknown type
				return;
			}
		}

		// Prepare selection
		if( nodeSelected )
		{
			brush.DeleteObject();
			brush.CreateSolidBrush(selcolor);
			pDC->SelectObject(&brush);
		}
		else
		{
			pDC->SelectObject(pOldBrush);
			pOldBrush = 0L;
		}

        bool isWorkingCopy = node->GetClassification().Is (CNodeClassification::IS_WORKINGCOPY);
        pen2.CreatePen(penStyle, isWorkingCopy ? 3 : 1, contour);
		pDC->SelectObject(&pen2);

		// Draw the main shape
		switch( shape )
		{
		case TSVNRectangle:
			pDC->Rectangle(rect);
			break;
		case TSVNRoundRect:
			pDC->RoundRect(rect, m_RoundRectPt);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, rect);
			break;
		case TSVNEllipse:
			pDC->Ellipse(rect);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}
		
		COLORREF brightcol = contour;
		int rval = (GetRValue(background)-GetRValue(contour))*9/10;
		int gval = (GetGValue(background)-GetGValue(contour))*9/10;
		int bval = (GetBValue(background)-GetBValue(contour))*9/10;
		brightcol = RGB(minmax(rval, GetRValue(contour)+rval, GetRValue(background)),
						minmax(gval, GetGValue(contour)+gval, GetGValue(background)),
						minmax(bval, GetBValue(contour)+bval, GetBValue(background)));

		brush.DeleteObject();
        brush.CreateSolidBrush(nodeSelected ? selcolor : brightcol);
		pOldBrush = pDC->SelectObject(&brush);

		// Draw the main shape
		switch( shape )
		{
		case TSVNRectangle:
			pDC->Rectangle(rect);
			break;
		case TSVNRoundRect:
			pDC->RoundRect(rect, m_RoundRectPt);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, rect);
			break;
		case TSVNEllipse:
			pDC->Ellipse(rect);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}
		if (m_nIconSize)
		{
			// draw the icon
			CPoint iconpoint = CPoint(rect.left + m_nIconSize/6, rect.top + m_nIconSize/6);
			CSize iconsize = CSize(m_nIconSize, m_nIconSize);
			pDC->DrawState(iconpoint, iconsize, hIcon, DST_ICON, (HBRUSH)NULL);
		}
		// Cleanup
		if (pOldPen != 0L)
		{
			pDC->SelectObject(pOldPen);
			pOldPen = 0L;
		}

		if (pOldBrush != 0L)
		{
			pDC->SelectObject(pOldBrush);
			pOldBrush = 0L;
		}

		pen.DeleteObject();
		pen2.DeleteObject();
		brush.DeleteObject();
	}
	CATCH_ALL(e)
	{
		if( pOldPen != 0L )
			pDC->SelectObject(pOldPen);

		if( pOldBrush != 0L )
			pDC->SelectObject(pOldBrush);
	}
	END_CATCH_ALL
}

void CRevisionGraphWnd::DrawNodes (CDC* pDC, const CRect& logRect, const CSize& offset)
{
    // load icons

	HICON hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_DELETED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedWithHistoryIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDEDPLUS), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_REPLACED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hRenamedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_RENAMED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hLastCommitIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_LASTCOMMIT), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hTaggedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_TAGGED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);

    // iterate over all visible nodes

    const ILayoutNodeList* nodes = m_layout->GetNodes();
    for ( index_t index = nodes->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = nodes->GetNextVisible (index, logRect))
	{
        // get node and position

        ILayoutNodeList::SNode node = nodes->GetNode (index);
		CRect noderect ( (int)(node.rect.left * m_fZoomFactor) - offset.cx
                       , (int)(node.rect.top * m_fZoomFactor) - offset.cy
                       , (int)(node.rect.right * m_fZoomFactor) - offset.cx
                       , (int)(node.rect.bottom * m_fZoomFactor) - offset.cy);

        // actual drawing

		switch (node.style)
		{
/*		case CRevisionEntry::deleted:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::DeletedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hDeletedIcon);
			break;
		case CRevisionEntry::added:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::AddedNode), entry, TSVNRoundRect, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hAddedIcon);
			break;
		case CRevisionEntry::addedwithhistory:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::AddedNode), entry, TSVNRoundRect, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hAddedWithHistoryIcon);
			break;
		case CRevisionEntry::replaced:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::ReplacedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hReplacedIcon);
			break;
		case CRevisionEntry::renamed:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::RenamedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hRenamedIcon);
			break;
		case CRevisionEntry::lastcommit:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::LastCommitNode), entry, TSVNEllipse, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hLastCommitIcon);
			break;*/
		default:
            DrawNode(pDC, noderect, GetSysColor(COLOR_WINDOWTEXT), node.node, TSVNRectangle, NULL);
			break;
		}

    	// Draw the "tagged" icon

        if (m_nIconSize && (node.node->GetFirstTag() != NULL))
		{
			// draw the icon
			CPoint iconpoint = CPoint(noderect.right - 7*m_nIconSize/6, noderect.top + m_nIconSize/6);
			CSize iconsize = CSize(m_nIconSize, m_nIconSize);
			pDC->DrawState(iconpoint, iconsize, hTaggedIcon, DST_ICON, (HBRUSH)NULL);
		}
    }

    // destroy items

	DestroyIcon(hDeletedIcon);
	DestroyIcon(hAddedIcon);
	DestroyIcon(hAddedWithHistoryIcon);
	DestroyIcon(hReplacedIcon);
	DestroyIcon(hRenamedIcon);
	DestroyIcon(hLastCommitIcon);
	DestroyIcon(hTaggedIcon);
}

void CRevisionGraphWnd::DrawConnections (CDC* pDC, const CRect& logRect, const CSize& offset)
{
    enum {MAX_POINTS = 100};
    CPoint points[MAX_POINTS];

	CPen newpen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	CPen * pOldPen = pDC->SelectObject(&newpen);

    // iterate over all visible lines

    const ILayoutConnectionList* connections = m_layout->GetConnections();
    for ( index_t index = connections->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = connections->GetNextVisible (index, logRect))
	{
        // get connection and point position

        ILayoutConnectionList::SConnection connection 
            = connections->GetConnection (index);

        if (connection.numberOfPoints > MAX_POINTS)
            continue;

        for (index_t i = 0; i < connection.numberOfPoints; ++i)
        {
            points[i].x = (int)(connection.points[i].x * m_fZoomFactor) - offset.cx;
            points[i].y = (int)(connection.points[i].y * m_fZoomFactor) - offset.cy;
        }

		// draw the connection

		pDC->PolyBezier (points, connection.numberOfPoints);
	}

	pDC->SelectObject(pOldPen);
}

void CRevisionGraphWnd::DrawTexts (CDC* pDC, const CRect& logRect, const CSize& offset)
{
	COLORREF textcolor = GetSysColor(COLOR_WINDOWTEXT);
    if (m_nFontSize <= 0)
        return;

    // iterate over all visible nodes

    const ILayoutTextList* texts = m_layout->GetTexts();
    for ( index_t index = texts->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = texts->GetNextVisible (index, logRect))
	{
        // get node and position

        ILayoutTextList::SText text = texts->GetText (index);
		CRect textRect ( (int)(text.rect.left * m_fZoomFactor) - offset.cx
                       , (int)(text.rect.top * m_fZoomFactor) - offset.cy
                       , (int)(text.rect.right * m_fZoomFactor) - offset.cx
                       , (int)(text.rect.bottom * m_fZoomFactor) - offset.cy);

		// draw the revision text

		pDC->SetTextColor (textcolor);
		pDC->SelectObject (GetFont(FALSE, FALSE));
        pDC->ExtTextOut (textRect.left, textRect.top, ETO_CLIPPED, &textRect, text.text, NULL);
    }
}

void CRevisionGraphWnd::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	CDC * memDC = bDirectDraw
                ? pDC
                : new CMemDC(pDC);
	
	memDC->FillSolidRect(rect, GetSysColor(COLOR_WINDOW));
	memDC->SetBkMode(TRANSPARENT);

    // transform visible

    CSize offset (nHScrollPos, nVScrollPos);
    CRect logRect ( (int)(offset.cx / m_fZoomFactor)-1
                  , (int)(offset.cy / m_fZoomFactor)-1
                  , (int)((rect.Width() + offset.cx) / m_fZoomFactor) + 1
                  , (int)((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

    // draw the different components

    DrawNodes (memDC, logRect, offset);
    DrawConnections (memDC, logRect, offset);
    DrawTexts (memDC, logRect, offset);

	// find out which nodes are in the visible area of the client rect

	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview))
	{
		// draw the overview image rectangle in the top right corner
		CMemDC memDC2(memDC, true);
		memDC2.SetWindowOrg(0, 0);
		HBITMAP oldhbm = (HBITMAP)memDC2.SelectObject(&m_Preview);
		memDC->BitBlt(rect.Width()-m_previewWidth, 0, m_previewWidth, m_previewHeight, 
			&memDC2, 0, 0, SRCCOPY);
		memDC2.SelectObject(oldhbm);
		// draw the border for the overview rectangle
		m_OverviewRect.left = rect.Width()-m_previewWidth;
		m_OverviewRect.top = 0;
		m_OverviewRect.right = rect.Width();
		m_OverviewRect.bottom = m_previewHeight;
		memDC->DrawEdge(&m_OverviewRect, EDGE_BUMP, BF_RECT);
		// now draw a rectangle where the current view is located in the overview
		LONG width = m_previewWidth * rect.Width() / m_ViewRect.Width();
		LONG height = m_previewHeight * rect.Height() / m_ViewRect.Height();
		LONG xpos = nHScrollPos * m_previewWidth / m_ViewRect.Width();
		LONG ypos = nVScrollPos * m_previewHeight / m_ViewRect.Height();
		RECT tempRect;
		tempRect.left = rect.Width()-m_previewWidth+xpos;
		tempRect.top = ypos;
		tempRect.right = tempRect.left + width;
		tempRect.bottom = tempRect.top + height;
		// make sure the position rect is not bigger than the preview window itself
		::IntersectRect(&m_OverviewPosRect, &m_OverviewRect, &tempRect);
		memDC->SetROP2(R2_MASKPEN);
		HGDIOBJ oldbrush = memDC->SelectObject(GetStockObject(GRAY_BRUSH));
		memDC->Rectangle(&m_OverviewPosRect);
		memDC->SetROP2(R2_NOT);
		memDC->SelectObject(oldbrush);
		memDC->DrawEdge(&m_OverviewPosRect, EDGE_BUMP, BF_RECT);
	}

	if (!bDirectDraw)
		delete memDC;
}

void CRevisionGraphWnd::DrawRubberBand()
{
	CDC * pDC = GetDC();
	pDC->SetROP2(R2_NOT);
	pDC->SelectObject(GetStockObject(NULL_BRUSH));
	pDC->Rectangle(min(m_ptRubberStart.x, m_ptRubberEnd.x), min(m_ptRubberStart.y, m_ptRubberEnd.y), 
		max(m_ptRubberStart.x, m_ptRubberEnd.x), max(m_ptRubberStart.y, m_ptRubberEnd.y));
	ReleaseDC(pDC);
}

