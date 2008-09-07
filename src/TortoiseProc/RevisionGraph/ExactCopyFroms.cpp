#include "StdAfx.h"
#include "ExactCopyFroms.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CExactCopyFroms::CExactCopyFroms (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: This option is negated.

bool CExactCopyFroms::IsActive() const
{
    return !IsSelected();
}

// implement ICopyFilterOption: 
// keep the previous node, if we plan to remove the pure copy sources

ICopyFilterOption::EResult 
CExactCopyFroms::ShallRemove (const CFullGraphNode* node) const
{
    const CFullGraphNode* next = node->GetNext();

    // next node has no "M", "A", "D" nor "R"
    // -> keep *this* node
    // (the "next" node will be removed in the second phase)

    return (   (next != NULL)
            && next->GetClassification().Matches (0, CNodeClassification::IS_OPERATION_MASK))
         ? ICopyFilterOption::PRESERVE_NODE
         : ICopyFilterOption::KEEP_NODE;
}

// implement IModificationOption:
// remove the pure copy sources

void CExactCopyFroms::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // remove node, if it is neither "M", "A", "D" nor "R"

    if (node->GetClassification().Matches (0, CNodeClassification::IS_OPERATION_MASK))
    {
        node->DropNode (graph);
    }
}
