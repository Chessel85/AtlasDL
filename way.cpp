//Way.cpp

#include "Way.h"
#include <QDebug>
#include "node.h"

CWay::CWay( quint64 id )
    : CBaseOSMObject( id ) 
{
}

CWay::~CWay()
{
}


CNode* CWay::getFirstNode()
{
    return m_nodes.first();
}

CNode*      CWay::getLastNode()
{
    return m_nodes.last();
}

void CWay::setBoundingBox(double south, double west, double  north, double east)
{
    m_boundingBox.setBoundingBox(south, west, north, east);
}

CBoundingBox& CWay::getBoundingBox() 
{
return m_boundingBox;
}

void CWay::addNode(CNode* node)
{
    m_nodes.append(node);
}

NodeList& CWay::getNodeList()
{
    return m_nodes;
}

void CWay::constructNodesWkt(QString& nodesWkt)
{
    nodesWkt += "ST_GeomFromText('LINESTRING(";
    //Do first node to make the loop later easier 
    if (m_nodes.size() > 0)
    {
        CNode* node = m_nodes.first();
        nodesWkt+= node->getLongitude() + " " + node->getLatitude();
    }

    //Add on remaining nodes including separating comma
    for (int i = 1; i < m_nodes.size(); i++)
    {
        CNode* node = m_nodes.at(i);
        nodesWkt+= "," + node->getLongitude() + " " + node->getLatitude();
    }

    //Attach the closing text
    nodesWkt+= ")',4326)";
}

void CWay::dumpInfo()
{
    quint64 id = getId();
    quint64 firstNodeId = getFirstNode()->getId();
    quint64 lastNodeId = getLastNode()->getId();
    int numNodes = m_nodes.size();

    qDebug() << "Way" << id << ", first node id" << firstNodeId << ",last node id" << lastNodeId << "with num nodes" << numNodes;
}