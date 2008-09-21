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
        subTreeMinY = max ( subTreeMinY
            , localColumnHeights[i+1] - branchColumnStarts[i] + node->rect.Height() / 2 + 8);
    }

    // store how much the sub-tree has to be shifted
    // (will be applied to .rect in a second pass)

    node->subTreeShift.cx = node->requiredSize.cx + 50;
    node->subTreeShift.cy = max (branchMinY, subTreeMinY);

    // adjust y-coord of the start node

    long nodeYShift = node->subTreeShift.cy - node->requiredSize.cy;
    node->rect.top += nodeYShift;
    node->rect.bottom += nodeYShift;

    // update column heights

    long subTreeYShift = node->subTreeShift.cy;
    localColumnStarts[0] = min (localColumnStarts[0], node->rect.top);
    localColumnHeights[0] = max (localColumnHeights[0], node->rect.top + node->requiredSize.cy);

    for (size_t i = 0, count = branchColumnStarts.size(); i < count; ++i)
    {
        localColumnStarts[i+1] = min ( localColumnStarts[i+1]
                                     , subTreeYShift + branchColumnStarts[i]);
        localColumnHeights[i+1] = max ( localColumnHeights[i+1]
                                      , subTreeYShift + branchColumnHeights[i]);
    }
}

void CStandardNodePositioning::AppendBranch 
    ( CStandardLayoutNodeInfo* start
    , std::vector<long>& columnStarts
    , std::vector<long>& columnHeights
    , std::vector<long>& localColumnStarts
    , std::vector<long>& localColumnHeights)
{
    // move the whole branch to the right

    start->treeShift.cx = columnStarts.size() * 250;
    
    // just append the column y-ranges 
    // (column 0 is for the chain that starts at node)

    columnStarts.insert ( columnStarts.end()
                        , localColumnStarts.begin()
                        , localColumnStarts.end());
    columnHeights.insert ( columnHeights.end()
                         , localColumnHeights.begin()
                         , localColumnHeights.end());
}

void CStandardNodePositioning::PlaceBranch 
    ( CStandardLayoutNodeInfo* start
    , std::vector<long>& columnStarts
    , std::vector<long>& columnHeights)
{
    // lower + upper bounds for the start node and all its branche columns

    std::vector<long> localColumnStarts;
    std::vector<long> localColumnHeights;

    // lower + upper bounds for the columns of one sub-branch of the start node

    std::vector<long> branchColumnStarts;
    std::vector<long> branchColumnHeights;

    for ( CStandardLayoutNodeInfo* node = start
        ; node != NULL
        ; node = node->nextInBranch)
    {
        // collect branches

        branchColumnStarts.clear();
        branchColumnHeights.clear();

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            PlaceBranch (branch, branchColumnStarts, branchColumnHeights);
        }

        // stack them and this node

        size_t subTreeWidth = branchColumnHeights.size()+1;
        if (localColumnStarts.size() < subTreeWidth)
        {
            localColumnStarts.resize (subTreeWidth, LONG_MAX);
            localColumnHeights.resize (subTreeWidth, 0);
        }

        StackSubTree ( node
                     , branchColumnStarts, branchColumnHeights
                     , localColumnStarts, localColumnHeights);
    }

    // append node and branchs horizontally to sibblings of the start node

    AppendBranch ( start
                 , columnStarts, columnHeights
                 , localColumnStarts, localColumnHeights);
}

void CStandardNodePositioning::ShiftNodes 
    ( CStandardLayoutNodeInfo* node
    , CSize delta)
{
    // walk along this branch

    delta += node->treeShift;
    for ( ; node != NULL; node = node->nextInBranch)
    {
        node->rect += delta;
        node->subTreeShift += delta;

        // shift sub-branches

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            ShiftNodes (branch, node->subTreeShift);
        }
    }
}

CRect CStandardNodePositioning::BoundingRect 
    (const CStandardLayoutNodeInfo* node)
{
    // walk along this branch

    CRect result = node->rect;
    for ( ; node != NULL; node = node->nextInBranch)
    {
        result.UnionRect (result, node->rect);

        // shift sub-branches

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            result.UnionRect (result, BoundingRect (branch));
        }
    }

    return result;
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

    // calculate the displacement for every node (member subTreeShift)

    CSize treeShift (0,0);
    for (index_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);
        if (   (node->node->GetPrevious() == NULL)
            && (node->node->GetCopySource() == NULL))
        {
            // we found a root -> place it

            std::vector<long> columnStarts;
            std::vector<long> columnHeights;

            PlaceBranch (node, columnStarts, columnHeights);

            // actually move the node rects to thier final position

            ShiftNodes (node, treeShift);
            treeShift.cx = BoundingRect (node).right + 50;
        }
    }
}
