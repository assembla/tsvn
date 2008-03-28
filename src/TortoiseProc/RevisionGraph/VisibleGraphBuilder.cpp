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
#include "VisibleGraphBuilder.h"
#include "FullGraph.h"
#include "VisibleGraph.h"
#include "CopyFilterOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// construction / destruction (nothing to do here)

CVisibleGraphBuilder::CVisibleGraphBuilder 
    ( const CFullGraph& fullGraph
    , CVisibleGraph& visibleGraph
    , const CCopyFilterOptions& copyFilter)
    : fullGraph (fullGraph)
    , visibleGraph (visibleGraph)
    , copyFilter (copyFilter)
{
}

CVisibleGraphBuilder::~CVisibleGraphBuilder(void)
{
}

// copy

void CVisibleGraphBuilder::Run()
{
    assert (visibleGraph.GetRoot() == NULL);

    if (fullGraph.GetRoot() == NULL)
        return;

    Copy (fullGraph.GetRoot(), NULL);
}

// the actual copy loop

void CVisibleGraphBuilder::Copy (const CFullGraphNode* source, CVisibleGraphNode* target)
{
    do
    {
        ICopyFilterOption::EResult filterAction = copyFilter.ShallRemove (source);
        while (filterAction != ICopyFilterOption::KEEP_NODE)
        {
            // stop copy here, if the whole sub-tree shall be removed

            if (filterAction == ICopyFilterOption::REMOVE_SUBTREE)
                return;

            // skip this node

            assert (filterAction == ICopyFilterOption::REMOVE_NODE);
            source = source->GetNext();

            // end of branch?

            if (source == NULL)
                return;
        }

        // copy the node itself

        target = visibleGraph.Add (source, target);

        // copy branches

        for ( const CFullGraphNode::CCopyTarget* copy = source->GetFirstCopyTarget()
            ; copy != NULL
            ; copy = copy->next())
        {
            Copy (copy->value(), target);
        }

        // next node

        source = source->GetNext();
    }
    while (source != NULL);
}
