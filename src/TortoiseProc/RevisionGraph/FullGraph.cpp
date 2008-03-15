#include "StdAfx.h"
#include "FullGraph.h"

// construction / destruction

CFullGraph::CFullGraph()
    : nodeFactory()
    , root (NULL)
    , nodeCount (0)
{
}

CFullGraph::~CFullGraph()
{
    if (root)
        nodeFactory.Destroy (root);
}

// modification

CFullGraphNode* CFullGraph::Add ( const CDictionaryBasedTempPath& path
                                , revision_t revision
                                , CNodeClassification classification
                                , CFullGraphNode* source)
{
    // (only) the first node must have no parent / prev node

    assert ((source == NULL) == (root == NULL));

    CFullGraphNode* result 
        = nodeFactory.Create (path, revision, classification, source);

    ++nodeCount;
    if (root == NULL)
        root = result;

    return result;
}

