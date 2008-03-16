#include "StdAfx.h"
#include "VisibleGraph.h"

// construction / destruction

CVisibleGraph::CVisibleGraph()
    : nodeFactory()
    , root (NULL)
    , nodeCount (0)
{
}

CVisibleGraph::~CVisibleGraph()
{
    if (root)
        nodeFactory.Destroy (root);
}

// modification

CVisibleGraphNode* CVisibleGraph::Add ( const CFullGraphNode* base
                                      , CVisibleGraphNode* source)
{
    // (only) the first node must have no parent / prev node

    assert ((source == NULL) == (root == NULL));

    CVisibleGraphNode* result 
        = nodeFactory.Create (base, source);

    ++nodeCount;
    if (root == NULL)
        root = result;

    return result;
}

