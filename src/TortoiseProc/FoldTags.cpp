#include "StdAfx.h"
#include "FoldTags.h"
#include "VisibleGraphNode.h"

// construction

CFoldTags::CFoldTags (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IModificationOption

void CFoldTags::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // we won't fold this node, if there are modifications on this
    // or any of its sub-branches.
    // Also, we will not remove tags that get copied to non-tags.

    DWORD forbidden = CNodeClassification::IS_MODIFIED
                    | CNodeClassification::COPIES_TO_MODIFIED 
                    | CNodeClassification::COPIES_TO_TRUNK 
                    | CNodeClassification::COPIES_TO_BRANCH 
                    | CNodeClassification::COPIES_TO_OTHER;

    // fold tags at the point of their creation

    if (   (node->GetCopySource() != NULL)
        && node->GetClassification().Matches (CNodeClassification::IS_TAG, forbidden))
    {
        node->FoldTag (graph);
    }
}
