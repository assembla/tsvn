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
#include "RevisionGraphOptions.h"
#include "VisibleGraph.h"
#include "VisibleGraphNode.h"

// called by CRevisionGraphOption constructor

void CRevisionGraphOptionList::Add (IRevisionGraphOption* option)
{
    options.push_back (option);
}

// utility method

IRevisionGraphOption* CRevisionGraphOptionList::GetOptionByID (UINT id) const
{
    for (size_t i = 0, count = options.size(); i < count; ++i)
        if (options[i]->CommandID() == id)
            return options[i];

    throw std::exception ("unknown option command ID");
}

// construction / destruction (frees all owned options)

CRevisionGraphOptionList::CRevisionGraphOptionList()
{
}

CRevisionGraphOptionList::~CRevisionGraphOptionList()
{
    for (size_t i = 0; i < options.size(); ++i)
        delete options[i];
}

// menu interaction

bool CRevisionGraphOptionList::IsAvailable (UINT id) const
{
    return GetOptionByID (id)->IsAvailable();
}

bool CRevisionGraphOptionList::IsSelected (UINT id) const
{
    return GetOptionByID (id)->IsSelected();
}

void CRevisionGraphOptionList::ToggleSelection (UINT id)
{
    GetOptionByID (id)->ToggleSelection();
}

// registry encoding

DWORD CRevisionGraphOptionList::GetRegistryFlags() const
{
    DWORD result = 0;
    assert (options.size() <= 8 * sizeof (result));

    DWORD flag = 1;
    for (size_t i = 0, count = options.size(); i < count; ++i, flag *= 2)
    {
        if (options[i]->IsSelected())
            result += flag;
    }

    return result;
}

void CRevisionGraphOptionList::SetRegistryFlags (DWORD flags)
{
    for (size_t i = 0, count = options.size(); i < count; ++i, flags /= 2)
    {
        if (((flags & 1) != 0) != options[i]->IsSelected())
            options[i]->ToggleSelection();
    }

    // no flags set for unknown options

    assert (flags == 0);
}

