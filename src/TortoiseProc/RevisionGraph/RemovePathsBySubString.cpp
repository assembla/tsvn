#include "StdAfx.h"
#include "RemovePathsBySubString.h"
#include "FullGraphNode.h"

// construction

CRemovePathsBySubString::CRemovePathsBySubString (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// get / set limits

/// access to the sub-string container

const std::set<std::string>& CRemovePathsBySubString::GetFilterPaths() const
{
    return filterPaths;
}

std::set<std::string>& CRemovePathsBySubString::GetFilterPaths()
{
    return filterPaths;
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemovePathsBySubString::ShallRemove (const CFullGraphNode* node) const
{
    // short-cut

    if (filterPaths.empty())
        return ICopyFilterOption::KEEP_NODE;

    // node to classify

    const CDictionaryBasedPath& path = node->GetRealPath();

    // ensure the index is valid within classification cache 

    if (pathClassification.size() <= path.GetIndex())
    {
        size_t newSize = max (8, pathClassification.size()) * 2;
        while (newSize <= path.GetIndex())
            newSize *= 2;

        pathClassification.resize (newSize, UNKNOWN);
    }

    // auto-calculate the entry

    PathClassification& classification = pathClassification[path.GetIndex()];
    if (classification == UNKNOWN)
    {
        std::string fullPath = path.GetPath();

        classification = KEEP;
    	for ( std::set<std::string>::const_iterator iter = filterPaths.begin()
            , end = filterPaths.end()
            ; iter != end
            ; ++iter)
        {
            if (fullPath.find (*iter) != std::string::npos)
            {
                classification = REMOVE;
                break;
            }
        }
    }

    // return the result

    return classification == REMOVE
        ? ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}

void CRemovePathsBySubString::Prepare()
{
    pathClassification.clear();
}
