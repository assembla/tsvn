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
#include "StandardNodePositioning.h"
#include "StandardLayout.h"

#include "VisibleGraph.h"

void CStandardNodePositioning::StackSubTree 
    ( CStandardLayoutNodeInfo* node
    , std::vector<long>& branchColumnStarts
    , std::vector<long>& branchColumnHeights
    , std::vector<long>& localColumnStarts
    , std::vector<long>& localColumnHeights)
{
    // the highest position allowed for any branch
    // (if actually reached, node must be at 0,0)

    long branchMinY = localColumnHeights[0] + node->requiredSize.cy;

    // shift subtree downwards until there is no overlap with upper sub-trees

    long subTreeMinY = 0;
    for (size_t i = 0, count = branchColumnStarts.size(); i < count; ++i)
    {
        subTreeMinY = min ( subTreeMinY
                          , localColumnHeights[i] - branchColumnStarts[i] + 10);
    }

    // store how much the sub-tree has to be shifted
    // (will be applied to rect in a second pass)

    node->subTreeShift.cx = node->requiredSize.cx + 50;
    node->subTreeShift.cy = max (branchMinY, subTreeMinY);

    // adjust y-coord of the start node

    long nodeYShift = node->subTreeShift.cy - branchMinY;
    node->rect.top += nodeYShift;
    node->rect.bottom += nodeYShift;

    // update column heights

    subTreeMinY = 0;
    for (size_t i = 0, count = branchColumnStarts.size(); i < count; ++i)
    {
        branchColumnStarts[i] = min ( branchColumnStarts[i]
                                    , subTreeMinY + branchColumnStarts[i]);
        localColumnHeights[i] = max ( localColumnHeights[i]
                                    , subTreeMinY + branchColumnHeights[i]);
    }
}

void CStandardNodePositioning::AppendBranch 
    ( CStandardLayoutNodeInfo* node
    , std::vector<long>& columnStarts
    , std::vector<long>& columnHeights
    , const std::vector<long>& localColumnStarts
    , const std::vector<long>& localColumnHeights)
{
}

void CStandardNodePositioning::PlaceBranch 
    ( CStandardLayoutNodeInfo* start
    , std::vector<long>& columnStarts
    , std::vector<long>& columnHeights)
{
    std::vector<long> localColumnStarts;
    localColumnStarts.resize (start->subTreeWidth, LONG_MAX);
    std::vector<long> localColumnHeights;
    localColumnHeights.resize (start->subTreeWidth, 0);

    std::vector<long> branchColumnStarts;
    std::vector<long> branchColumnHeights;

    for ( CStandardLayoutNodeInfo* node = start
        ; node != NULL
        ; node = node->nextInBranch)
    {
        branchColumnStarts.clear();
        localColumnStarts.resize (node->subTreeWidth-1, LONG_MAX);
        branchColumnHeights.clear();
        localColumnHeights.resize (node->subTreeWidth-1, 0);

        // add branches

        for ( CStandardLayoutNodeInfo* branch = node->lastBranch
            ; branch != NULL
            ; branch = branch->previousBranch)
        {
            PlaceBranch (branch, branchColumnStarts, branchColumnHeights);
        }

        // stack them and this node

        StackSubTree ( node
                     , branchColumnStarts, branchColumnHeights
                     , localColumnStarts, localColumnHeights);
    }

    // append branch horizontally to sibblings

    AppendBranch ( start
                 , columnStarts, columnHeights
                 , localColumnStarts, localColumnHeights);
}

// construction

CStandardNodePositioning::CStandardNodePositioning 
    ( CRevisionGraphOptionList& list)
    : CRevisionGraphOptionImpl<ILayoutOption, 200, 0> (list)
{
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CStandardNodePositioning::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // run

    for (index_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);
        node->requiredSize = CSize (200, 60);
    }
}
