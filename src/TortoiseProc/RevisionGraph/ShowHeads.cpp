#include "StdAfx.h"
#include "ShowHeads.h"
#include "FullGraphNode.h"

// construction

CShowHead::CShowHead (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CShowHead::ShallRemove (const CFullGraphNode* node) const
{
    // "Pin" HEAD nodes

    return node->GetClassification().Is (CNodeClassification::IS_LAST)
         ? ICopyFilterOption::PRESERVE_NODE
         : ICopyFilterOption::KEEP_NODE;
}
