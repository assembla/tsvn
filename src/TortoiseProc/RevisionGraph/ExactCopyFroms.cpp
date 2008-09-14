#include "StdAfx.h"
#include "ExactCopyFroms.h"
#include "VisibleGraphNode.h"

// construction

CExactCopyFroms::CExactCopyFroms (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: This option must always be applied.

bool CExactCopyFroms::IsActive() const
{
    return true;
}

// implement IModificationOption:
// remove the pure copy sources

void CExactCopyFroms::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // remove node, if it is neither "M", "A", "D" nor "R"

    if (node->GetClassification().Matches (0, CNodeClassification::IS_OPERATION_MASK))
    {
        // is this node still necessary?

        CVisibleGraphNode* next = node->GetNext();

        bool isCopySource =    (node->GetFirstTag() != NULL)
                            || (node->GetFirstCopyTarget() != NULL)
                            || (   (next != NULL) 
                                && (next->GetClassification().Is 
                                        (CNodeClassification::IS_RENAMED)));

        // remove it, if it is either no longer necessary or not wanted at all

        if (!IsSelected() || !isCopySource)
            node->DropNode (graph);
    }
}
