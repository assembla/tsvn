#include "StdAfx.h"
#include "RevisionInRange.h"
#include "FullGraphNode.h"

// construction

CRevisionInRange::CRevisionInRange (CRevisionGraphOptionList& list)
    : inherited (list)
    , lowerLimit (NO_REVISION)
    , upperLimit (NO_REVISION)
{
}

// get / set limits

revision_t CRevisionInRange::GetLowerLimit() const
{
    return lowerLimit;
}

void CRevisionInRange::SetLowerLimit (revision_t limit)
{
    lowerLimit = limit;
}

revision_t CRevisionInRange::GetUpperLimit() const
{
    return upperLimit;
}

void CRevisionInRange::SetUpperLimit (revision_t limit)
{
    upperLimit = limit;
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRevisionInRange::ShallRemove (const CFullGraphNode* node) const
{
    // out of revision range?

    if ((lowerLimit != NO_REVISION) && (node->GetRevision() < lowerLimit))
        return ICopyFilterOption::REMOVE_NODE;

    if ((upperLimit != NO_REVISION) && (node->GetRevision() > upperLimit))
        return ICopyFilterOption::REMOVE_NODE;

    return ICopyFilterOption::KEEP_NODE;
}
