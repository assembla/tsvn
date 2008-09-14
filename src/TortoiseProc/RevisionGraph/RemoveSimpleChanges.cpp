#include "StdAfx.h"
#include "RemoveSimpleChanges.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CRemoveSimpleChanges::CRemoveSimpleChanges (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: This option is negated.

bool CRemoveSimpleChanges::IsActive() const
{
    return !IsSelected();
}

// implement ICopyFilterOption (pre-filter most nodes)

ICopyFilterOption::EResult 
CRemoveSimpleChanges::ShallRemove (const CFullGraphNode* node) const
{
    // "M", not a branch point and will not become one due to removed copy-froms

    bool nodeIsModificationOnly
        =    (node->GetClassification().Is (CNodeClassification::IS_MODIFIED))
          && (node->GetFirstCopyTarget() == NULL);

    const CFullGraphNode* next = node->GetNext();
    bool nextIsModificationOnly
        =    (next != NULL)
          && (next->GetClassification().Is (CNodeClassification::IS_MODIFIED))
          && (next->GetFirstCopyTarget() == NULL);

    return nodeIsModificationOnly && nextIsModificationOnly
        ? ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}

// implement IModificationOption (post-filter unused copy-from nodes)

void CRemoveSimpleChanges::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // "M", not a branch point, not the HEAD

    if (   (node->GetClassification().Matches ( CNodeClassification::IS_MODIFIED
                                              , CNodeClassification::MUST_BE_PRESERVED))
        && (node->GetFirstTag() == NULL)
        && (node->GetFirstCopyTarget() == NULL))
    {
        node->DropNode (graph);
    }
}
