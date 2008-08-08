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
#pragma once

#include "VisibleGraphNode.h"

class CVisibleGraph
{
private:

    CVisibleGraphNode::CFactory nodeFactory;

    /// the graph is actually a tree

    CVisibleGraphNode* root;
    size_t nodeCount;

public:

    /// construction / destruction

    CVisibleGraph(void);
    ~CVisibleGraph(void);

    /// modification

    void Clear();
    CVisibleGraphNode* Add ( const CFullGraphNode* base
                           , CVisibleGraphNode* source);

    /// member access

    CVisibleGraphNode* GetRoot();
    const CVisibleGraphNode* GetRoot() const;

    size_t GetNodeCount() const;
};

/// member access

inline const CVisibleGraphNode* CVisibleGraph::GetRoot() const
{
    return root;
}

inline CVisibleGraphNode* CVisibleGraph::GetRoot()
{
    return root;
}

inline size_t CVisibleGraph::GetNodeCount() const
{
    return nodeCount;
}
