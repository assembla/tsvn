#include "StdAfx.h"
#include "FoldTags.h"

// construction

CFoldTags::CFoldTags (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IModificationOption

void CFoldTags::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
}
