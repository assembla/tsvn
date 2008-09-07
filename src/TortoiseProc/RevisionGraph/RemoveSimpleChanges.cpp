#include "StdAfx.h"
#include "RemoveSimpleChanges.h"
#include "FullGraphNode.h"

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

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemoveSimpleChanges::ShallRemove (const CFullGraphNode* node) const
{
    // "M", not a branch point, not the HEAD

    return     (node->GetClassification().Is (CNodeClassification::IS_MODIFIED))
            && (node->GetFirstCopyTarget() == NULL)
        ? ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}
