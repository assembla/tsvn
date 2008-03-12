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

#include "FullGraphNode.h"

/**
 * \ingroup TortoiseProc
 * Helper class, representing a revision with all the required information
 * which we need to draw a revision graph.
 */
class CVisibleGraphNode
{
public:

    class CFoldedTag
    {
    private:

        const CFullGraphNode* tagNode;
        CFoldedTag* next;
        size_t depth;

    public:

        /// construction

	    CFoldedTag ( const CFullGraphNode* tagNode
                   , size_t depth
                   , CFoldedTag* next = NULL);

        /// data access

        const CFullGraphNode* GetTag() const;
        const CFoldedTag* GetNext() const;
        size_t GetDepth() const;

        bool IsAlias() const;
        bool IsDeleted() const;
    };

    /// copy target list type

    typedef simple_list<CVisibleGraphNode> CCopyTarget;

    /// factory type

    class CFactory
    {
    private:

        boost::pool<> nodePool;
        CCopyTarget::factory copyTargetFactory;

    public:

        /// factory creation

        CFactory();

        /// factory interface

        CVisibleGraphNode* Create ( const CFullGraphNode* base
                                  , CVisibleGraphNode* prev);
        void Destroy (CVisibleGraphNode* node);
    };

    friend class CFactory;

private:

	/// members

    const CFullGraphNode* base;

	CCopyTarget*	    firstCopyTarget;
    CFoldedTag*         firstTag;

	CVisibleGraphNode*  prev;
	CVisibleGraphNode*  next;

	CVisibleGraphNode*  copySource;

    CNodeClassification classification;

	int				    column;
	int				    row;

    /// construction / destruction via pool

protected:

	/// protect construction / destruction to force usage of pool

	CVisibleGraphNode ( const CFullGraphNode* base
                      , CVisibleGraphNode* prev
                      , CCopyTarget::factory& copyTargetFactory);
    ~CVisibleGraphNode();

    /// destruction utility

    void DestroySubNodes ( CFactory& factory
                         , CCopyTarget::factory& copyTargetFactory);

public:

    /// data access

	const CDictionaryBasedTempPath& GetPath() const;
	CDictionaryBasedPath GetRealPath() const;
	const CFoldedTag* GetFirstTag() const;

	const CVisibleGraphNode* GetCopySource() const;
    const CCopyTarget* GetFirstCopyTarget() const;

	const CVisibleGraphNode* GetPrevious() const;
	const CVisibleGraphNode* GetNext() const;

	revision_t GetRevision() const;
	CNodeClassification GetClassification() const;

	int GetColumn() const;
	int	GetRow() const;
};

/// CVisibleGraphNode::CFoldedTag construction

inline CVisibleGraphNode::CFoldedTag::CFoldedTag 
    ( const CFullGraphNode* tagNode
    , size_t depth
    , CFoldedTag* next)
	: tagNode (tagNode), depth (depth), next (next)
{
}

/// CVisibleGraphNode::CFoldedTag data access

inline const CFullGraphNode* CVisibleGraphNode::CFoldedTag::GetTag() const
{
    return tagNode;
}

inline const CVisibleGraphNode::CFoldedTag* 
CVisibleGraphNode::CFoldedTag::GetNext() const
{
    return next;
}

inline size_t CVisibleGraphNode::CFoldedTag::GetDepth() const
{
    return depth;
}

inline bool CVisibleGraphNode::CFoldedTag::IsDeleted() const
{
    return tagNode->GetClassification()
        .Is (CNodeClassification::SUBTREE_DELETED);
}

/// CVisibleGraphNode data access

inline const CDictionaryBasedTempPath& CVisibleGraphNode::GetPath() const
{
    return base->GetPath();
}

inline CDictionaryBasedPath CVisibleGraphNode::GetRealPath() const
{
    return base->GetRealPath();
}

inline const CVisibleGraphNode::CFoldedTag* CVisibleGraphNode::GetFirstTag() const
{
    return firstTag;
}

inline const CVisibleGraphNode* CVisibleGraphNode::GetCopySource() const
{
    return copySource;
}

inline const CVisibleGraphNode::CCopyTarget* 
CVisibleGraphNode::GetFirstCopyTarget() const
{
    return firstCopyTarget;
}

inline const CVisibleGraphNode* CVisibleGraphNode::GetPrevious() const
{
    return prev;
}

inline const CVisibleGraphNode* CVisibleGraphNode::GetNext() const
{
    return next;
}

inline revision_t CVisibleGraphNode::GetRevision() const
{
    return base->GetRevision();
}

inline CNodeClassification CVisibleGraphNode::GetClassification() const
{
    return classification;
}

inline int CVisibleGraphNode::GetColumn() const
{
    return column;
}

inline int CVisibleGraphNode::GetRow() const
{
    return row;
}
