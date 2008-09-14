#include "StdAfx.h"
#include "StandardLayout.h"

#include "VisibleGraph.h"

#include "StandardLayoutNodeList.h"
#include "StandardLayoutConnectionList.h"
#include "StandardLayoutTextList.h"

// initialization

CStandardLayoutNodeInfo::CStandardLayoutNodeInfo()
    : node (NULL)
    , firstSubBranch (NULL)
    , nextInBranch (NULL)
    , previousInBranch (NULL)
    , lastInBranch (NULL)
    , nextBranch (NULL)
    , previousBranch (NULL)
    , lastBranch (NULL)
    , subTreeWidth (1)
    , subTreeHeight (1)
    , subTreeWeight (1)
    , branchLength (1)
    , requiresRevision (true)
    , requiresPath (true)
    , requiresGap (true)
    , requiredSize (0, 0)
    , subTreeShift (0, 0)
    , treeShift (0, 0)
    , rect (0, 0, 0, 0)
{
}

// layout creation: 
// * create a node info object for every node
// * calculate branch and tree sizes (in nodes)

void CStandardLayout::InitializeNodes (const CVisibleGraphNode* start)
{
    CStandardLayoutNodeInfo* previousInBranch = NULL;
    CStandardLayoutNodeInfo* lastInBranch = NULL;

    // measure subtree size, calculate branch length and back-link it

    index_t branchLength = 0;
    for ( const CVisibleGraphNode* node = start 
        ; node != NULL
        ; node = node->GetNext())
    {
        // get the node, initialize it and update pointers

        CStandardLayoutNodeInfo& nodeInfo = nodes[node->GetIndex()];
        ++branchLength;

        assert (nodeInfo.node == NULL);
        nodeInfo.node = node;
        nodeInfo.previousInBranch = previousInBranch;
        if (previousInBranch != NULL)
            previousInBranch->nextInBranch = &nodeInfo;
        
        previousInBranch = &nodeInfo;
        lastInBranch = &nodeInfo;

        if (node->GetFirstCopyTarget())
        {
            CStandardLayoutNodeInfo* previousBranch = NULL;
            CStandardLayoutNodeInfo* lastBranch = NULL;

            // measure sub-branches and back-link them 

            for ( const CVisibleGraphNode::CCopyTarget* 
                    target = node->GetFirstCopyTarget()
                ; target != NULL
                ; target = target->next())
            {
                // get sub-branch node, initialize it and update pointers

                const CVisibleGraphNode* subNode = target->value();
                CStandardLayoutNodeInfo& subNodeInfo = nodes[subNode->GetIndex()];

                if (previousBranch != NULL)
                {
                    previousBranch->nextBranch = &subNodeInfo;
                    subNodeInfo.previousBranch = previousBranch;
                }
                else
                {
                    // mark the first branch

                    nodeInfo.firstSubBranch = &subNodeInfo;
                }

                previousBranch = &subNodeInfo;
                lastBranch = &subNodeInfo;

                // add branch

                InitializeNodes (subNode);

                // accumulate branch into sub-tree

                nodeInfo.subTreeWidth += subNodeInfo.subTreeWidth;
                nodeInfo.subTreeWeight += subNodeInfo.subTreeWeight;
                if (nodeInfo.subTreeHeight <= subNodeInfo.subTreeHeight)
                    nodeInfo.subTreeHeight = subNodeInfo.subTreeHeight+1;
            }

            // link sub-branches forward

            for ( const CVisibleGraphNode::CCopyTarget* 
                    target = node->GetFirstCopyTarget()
                ; target != NULL
                ; target = target->next())
            {
                nodes[target->value()->GetIndex()].lastBranch = lastBranch;
            }
        }
    }

    // write branch lengths, adjust sub-tree heights and link forward

    for ( const CVisibleGraphNode* node = start 
        ; node != NULL
        ; node = node->GetNext())
    {
        CStandardLayoutNodeInfo& nodeInfo = nodes[node->GetIndex()];
        nodeInfo.branchLength = branchLength;
        nodeInfo.lastInBranch = lastInBranch;

        if (nodeInfo.subTreeHeight < branchLength)
            nodeInfo.subTreeHeight = branchLength;

        --branchLength;

        nodeInfo.subTreeWeight += branchLength;
    }
}

void CStandardLayout::InitializeNodes (const CVisibleGraph* graph)
{
    nodes.resize (graph->GetNodeCount());

    for (size_t i = 0, count = graph->GetRootCount(); i < count; ++i)
        InitializeNodes (graph->GetRoot (i));

    // every node info must actually refer to a node

    for (size_t i = 0, count = nodes.size(); i < count; ++i)
        assert (nodes[i].node != NULL);
}

// scan tree for connections between non-overlapping nodes

void CStandardLayout::CreateConnections()
{
    // there can't be more connections than nodes

    connections.reserve (nodes.size());

    // (root is at index 0)

    for (index_t i = 1, count = (index_t)nodes.size(); i < count; ++i)
    {
        const CVisibleGraphNode* node = nodes[i].node;
        const CVisibleGraphNode* previousNode = node->GetCopySource()
                                              ? node->GetCopySource()
                                              : node->GetPrevious();

        const CRect& previousRect = nodes[previousNode->GetIndex()].rect;
        CRect rect = nodes[i].rect;

        // no line because nodes touch or overlap?

        rect.InflateRect (1, 1, 1, 1);
        if (TRUE == CRect().IntersectRect (rect, previousRect))
            continue;

        // an actual connection

        connections.push_back (std::make_pair (previousNode->GetIndex(), i));
    }
}

// scan tree for connections longer than 0

void CStandardLayout::CreateTexts()
{
    // there can be at most two texts per node

    texts.reserve (2*nodes.size());

    // cover all node rects 
    // (connections and texts will lie within these bounds)

    for (index_t i = 0, count = (index_t)nodes.size(); i < count; ++i)
    {
        const CStandardLayoutNodeInfo& info = nodes[i];

        if (info.requiresRevision)
            texts.push_back (std::make_pair (i, false));
        if (info.requiresPath)
            texts.push_back (std::make_pair (i, true));
    }
}

// just iterate over all nodes

void CStandardLayout::CalculateBoundingRect()
{
    // special case: empty graph

    if (nodes.empty())
    {
        boundingRect = CRect();
        return;
    }

    // cover all node rects 
    // (connections and texts will lie within these bounds)

    boundingRect = nodes[0].rect;
    for (size_t i = 1, count = nodes.size(); i < count; ++i)
        boundingRect |= nodes[i].rect;
}

/// construction / destruction

CStandardLayout::CStandardLayout ( const CCachedLogInfo* cache
                                 , const CVisibleGraph* graph)
    : cache (cache)
{
    InitializeNodes (graph);
}

CStandardLayout::~CStandardLayout(void)
{
}

// call this after executing the format options

void CStandardLayout::Finalize()
{
    CreateConnections();
    CreateTexts();
    CalculateBoundingRect();
}

/// implement IRevisionGraphLayout

CRect CStandardLayout::GetRect() const
{
    return boundingRect;
}

const ILayoutNodeList* CStandardLayout::GetNodes() const
{
    return new CStandardLayoutNodeList (nodes, cache);
}

const ILayoutConnectionList* CStandardLayout::GetConnections() const
{
    return new CStandardLayoutConnectionList (nodes, connections);
}

const ILayoutTextList* CStandardLayout::GetTexts() const
{
    return new CStandardLayoutTextList (nodes, texts);
}

/// implement IStandardLayoutNodeAccess

index_t CStandardLayout::GetNodeCount() const
{
    return static_cast<index_t>(nodes.size());
}

CStandardLayoutNodeInfo* CStandardLayout::GetNode (index_t index)
{
    return &nodes[index];
}

