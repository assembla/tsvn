#include "StdAfx.h"
#include "VisibleGraph.h"

// construction / destruction

CVisibleGraph::CVisibleGraph()
    : nodeFactory()
{
}

CVisibleGraph::~CVisibleGraph()
{
    Clear();
}

// modification

void CVisibleGraph::Clear()
{
    for (size_t i = roots.size(); i > 0; --i)
    {
        nodeFactory.Destroy (roots[i-1]);
        assert (GetNodeCount() == 0);
    }

    roots.clear();
}

CVisibleGraphNode* CVisibleGraph::Add ( const CFullGraphNode* base
                                      , CVisibleGraphNode* source
                                      , bool preserveNode)
{
    // (only) the first node must have no parent / prev node

    assert ((source == NULL) || !roots.empty());

    CVisibleGraphNode* result 
        = nodeFactory.Create (base, source, preserveNode);

    if (source == NULL)
        roots.push_back (result);

    return result;
}

void CVisibleGraph::ReplaceRoot ( CVisibleGraphNode* oldRoot
                                , CVisibleGraphNode* newRoot)
{
    for (size_t i = 0, count = roots.size(); i < count; ++i)
        if (roots[i] == oldRoot)
        {
            roots[i] = newRoot;
            return;
        }

    // we should never get here

    assert (0);
}

void CVisibleGraph::RemoveRoot (CVisibleGraphNode* root)
{
    for (size_t i = 0, count = roots.size(); i < count; ++i)
        if (roots[i] == root)
        {
            roots[i] = roots[count-1];
            roots.pop_back();

            return;
        }

    // we should never get here

    assert (0);
}

