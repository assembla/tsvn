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
#include "ModificationOptions.h"
#include "VisibleGraph.h"
#include "VisibleGraphNode.h"

// apply a filter using differnt traversal orders

void CModificationOptions::TraverseFromRootCopiesFirst 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // copies first

    for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
        ; copy != NULL
        ; copy = copy->next())
	{
        TraverseFromRootCopiesFirst (option, graph, copy->value());
    }

    // node afterwards

    option->Apply (graph, node);

    // follow branch last

    if (node->GetNext() != NULL)
        TraverseFromRootCopiesFirst (option, graph, node->GetNext());
}

void CModificationOptions::TraverseToRootCopiesFirst 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // follow branch first

    if (node->GetNext() != NULL)
        TraverseToRootCopiesFirst (option, graph, node->GetNext());

    // copies second

    for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
        ; copy != NULL
        ; copy = copy->next())
	{
        TraverseToRootCopiesFirst (option, graph, copy->value());
    }

    // node last

    option->Apply (graph, node);
}

void CModificationOptions::TraverseFromRootCopiesLast 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // node first

    option->Apply (graph, node);

    // follow branch afterwards

    if (node->GetNext() != NULL)
        TraverseFromRootCopiesLast (option, graph, node->GetNext());

    // copies last

    for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
        ; copy != NULL
        ; copy = copy->next())
	{
        TraverseFromRootCopiesLast (option, graph, copy->value());
    }
}

void CModificationOptions::TraverseToRootCopiesLast 
    ( IModificationOption* option
    , CVisibleGraph* graph
    , CVisibleGraphNode* node)
{
    // follow branch first

    if (node->GetNext() != NULL)
        TraverseToRootCopiesLast (option, graph, node->GetNext());

    // node afterwards

    option->Apply (graph, node);

    // copies last

    for ( const CVisibleGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
        ; copy != NULL
        ; copy = copy->next())
	{
        TraverseToRootCopiesLast (option, graph, copy->value());
    }
}

// construction

CModificationOptions::CModificationOptions 
    ( const std::vector<IModificationOption*>& options)
    : options (options)
{
}

// apply all filters 

void CModificationOptions::Apply (CVisibleGraph* graph)
{
    typedef std::vector<IModificationOption*>::const_iterator IT;
    for ( IT iter = options.begin(), end = options.end()
        ; (iter != end)
        ; ++iter)
    {
        if ((*iter)->WantsCopiesFirst())
            if ((*iter)->WantsRootFirst())
                TraverseFromRootCopiesFirst (*iter, graph, graph->GetRoot());
            else
                TraverseToRootCopiesFirst (*iter, graph, graph->GetRoot());
        else
            if ((*iter)->WantsRootFirst())
                TraverseFromRootCopiesLast (*iter, graph, graph->GetRoot());
            else
                TraverseToRootCopiesLast (*iter, graph, graph->GetRoot());
    }
}

