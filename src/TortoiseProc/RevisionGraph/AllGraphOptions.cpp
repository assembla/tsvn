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

#include "StdAfx.h"
#include "AllGraphOptions.h"
#include "Resource.h"

#include "RemoveSimpleChanges.h"

#include "StandardNodeSizeAssignment.h"
#include "StandardNodePositioning.h"

// construction (create all option objects) / destruction

CAllRevisionGraphOptions::CAllRevisionGraphOptions()
{
    // create options.

    // The order is critical as it determines the option bit position
    // in the registry DWORD.

    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_GROUPBRANCHES> (*this);
    new CRemoveSimpleChanges (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_TOPDOWN> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_SHOWHEAD> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_REDUCECROSSLINES> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_EXACTCOPYSOURCE> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, 0> (*this);   // 0x40 is not used
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_FOLDTAGS> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_REMOVEDELETEDONES> (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_SHOWWCREV> (*this);

    // create layout options

    (new CStandardNodeSizeAssignment (*this))->ToggleSelection();
    (new CStandardNodePositioning (*this))->ToggleSelection();
}

// access specific sub-sets

CCopyFilterOptions CAllRevisionGraphOptions::GetCopyFilterOptions() const
{
    return GetFilteredList<CCopyFilterOptions, ICopyFilterOption>();
}

CModificationOptions CAllRevisionGraphOptions::GetModificationOptions() const
{
    return GetFilteredList<CModificationOptions, IModificationOption>();
}

CLayoutOptions CAllRevisionGraphOptions::GetLayoutOptions() const
{
    return GetFilteredList<CLayoutOptions, ILayoutOption>();
}
