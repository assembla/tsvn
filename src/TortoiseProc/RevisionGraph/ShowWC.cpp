#include "StdAfx.h"
#include "ShowWC.h"
#include "FullGraphNode.h"

// construction

CShowWC::CShowWC (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CShowWC::ShallRemove (const CFullGraphNode* node) const
{
    // "Pin" HEAD nodes

    return node->GetClassification().Is (CNodeClassification::IS_WORKINGCOPY)
         ? ICopyFilterOption::PRESERVE_NODE
         : ICopyFilterOption::KEEP_NODE;
}
