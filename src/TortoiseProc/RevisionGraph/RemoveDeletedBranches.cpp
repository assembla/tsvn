#include "StdAfx.h"
#include "RemoveDeletedBranches.h"
#include "FullGraphNode.h"

// construction

CRemoveDeletedBranches::CRemoveDeletedBranches (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemoveDeletedBranches::ShallRemove (const CFullGraphNode* node) const
{
    // "M", not a branch point, not the HEAD

    return node->GetClassification().Is (CNodeClassification::ALL_COPIES_DELETED)
         ? ICopyFilterOption::REMOVE_SUBTREE
         : ICopyFilterOption::KEEP_NODE;
}
