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
#include "VisibleGraphNode.h"

// CVisibleGraphNode::CFoldedTag methods

bool CVisibleGraphNode::CFoldedTag::IsAlias() const
{
    const CFullGraphNode* prev 
        = tagNode->GetPrevious() == NULL
            ? tagNode->GetCopySource()
            : tagNode->GetPrevious();

    return    (prev != NULL) 
           && prev->GetClassification().Is (CNodeClassification::IS_TAG)
           && prev->GetClassification().IsAnyOf (  CNodeClassification::IS_ADDED
                                                 + CNodeClassification::IS_RENAMED);
}

// CVisibleGraphNode::CFactory methods

CVisibleGraphNode::CFactory::CFactory()
    : nodePool (sizeof (CVisibleGraphNode), 1024)
    , copyTargetFactory()
{
}

CVisibleGraphNode* CVisibleGraphNode::CFactory::Create 
    ( const CFullGraphNode* base
    , CVisibleGraphNode* prev)
{
    CVisibleGraphNode * result = static_cast<CVisibleGraphNode *>(nodePool.malloc());
    new (result) CVisibleGraphNode (base, prev, copyTargetFactory);
    return result;
}

void CVisibleGraphNode::CFactory::Destroy (CVisibleGraphNode* node)
{
    node->DestroySubNodes (*this, copyTargetFactory);

    node->~CVisibleGraphNode();
    nodePool.free (node);
}

// CVisibleGraphNode construction / destruction 

CVisibleGraphNode::CVisibleGraphNode 
    ( const CFullGraphNode* base
    , CVisibleGraphNode* prev
    , CCopyTarget::factory& copyTargetFactory)
	: base (base)
	, firstCopyTarget (NULL), firstTag (NULL)
	, prev (NULL), next (NULL), copySource (NULL)
    , classification (base->GetClassification())
	, index (NO_INDEX) 
{
    if (prev != NULL)
        if (classification.Is (CNodeClassification::IS_COPY_TARGET))
        {
            copySource = prev;
            copyTargetFactory.insert (this, prev->firstCopyTarget);
        }
        else
        {
            assert (prev->next == NULL);

            prev->next = this;
            this->prev = prev;
        }
}

CVisibleGraphNode::~CVisibleGraphNode() 
{
    assert (next == NULL);
    assert (firstCopyTarget == NULL);
};

void CVisibleGraphNode::DestroySubNodes 
    ( CFactory& factory
    , CCopyTarget::factory& copyTargetFactory)
{
    while (next != NULL)
    {
        CVisibleGraphNode* toDestroy = next; 
        next = toDestroy->next;
        toDestroy->next = NULL;

        factory.Destroy (toDestroy);
    }

    while (firstCopyTarget)
    {
        factory.Destroy (copyTargetFactory.remove (firstCopyTarget));
    }
}

// set index members within the whole sub-tree

index_t CVisibleGraphNode::InitIndex (index_t startIndex)
{
    for (CVisibleGraphNode* node = this; node != NULL; node = node->next)
    {
        node->index = startIndex;
        ++startIndex;

        for ( CCopyTarget* target = node->firstCopyTarget
            ; target != NULL
            ; target = target->next())
        {
            startIndex = target->value()->InitIndex (startIndex);
        }
    }

    return startIndex;
}
