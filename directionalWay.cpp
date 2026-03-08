//directionalWay.cpp

#include "DirectionalWay.h"
#include "node.h"
#include "way.h"
#include <QDebug>

CDirectionalWay::CDirectionalWay(CWay* way, enumDirection direction)
{
    m_way = nullptr;
    m_wayId = 0;
    m_direction = forward;
    m_headNodeId = 0;
    m_tailNodeId = 0;

    set(way, direction);
}

CDirectionalWay::CDirectionalWay(PolygonWayEntry& pwe, CWay* way)
{
    m_way = way;
    m_wayId = pwe.wayId;
    m_direction = pwe.direction;
    //Head and tail nodes already correctly orientated in way entry so simply copy values 
    m_headNodeId = pwe.headNodeId;
    m_tailNodeId = pwe.tailNodeId;
}

CDirectionalWay::CDirectionalWay(CDirectionalWay* dw)
{
    m_way = dw->m_way;
    m_wayId = dw->m_wayId;
    m_direction = dw->m_direction;
    m_headNodeId = dw->m_headNodeId;
    m_tailNodeId = dw->m_tailNodeId;
}

CWay* CDirectionalWay::getWay()
{
    return m_way;
}

CNode* CDirectionalWay::getHeadNode()
{
    CNode* node = nullptr;

    if (!m_way)
        return node;

    if (m_direction == forward)
        node = m_way->getFirstNode();
    else 
        node = m_way->getLastNode();

    return node;
}

CNode* CDirectionalWay::getTailNode()
{
    CNode* node = nullptr;

    if (!m_way)
        return  node;

    if (m_direction == forward)
        node = m_way->getLastNode();
    else
        node = m_way->getFirstNode();

    return node;
}

quint64 CDirectionalWay::getWayId()
{
    return m_wayId;
}

quint64 CDirectionalWay::getHeadNodeId()
{
        return m_headNodeId;
}

quint64 CDirectionalWay::getTailNodeId()
{
    return m_tailNodeId;
}

enumDirection CDirectionalWay::getDirection()
{
    return m_direction;
}

void CDirectionalWay::set(CWay* way, enumDirection direction)
{
    m_way = way;
    if (way)
        m_wayId = way->getId();
    m_direction = direction;

    //Set the head and tail node id  accounting for direction 
    if (way)
    {
        if (direction == forward)
        {
            CNode* firstNode = way->getFirstNode();
            if (firstNode)
                m_headNodeId = firstNode->getId();
            CNode* lastNode = way->getLastNode();
            if (lastNode)
                m_tailNodeId = lastNode->getId();
        }
        else if (direction == backward)
        {
            CNode* firstNode = way->getFirstNode();
            if (firstNode)
                m_tailNodeId = firstNode->getId();
            CNode* lastNode = way->getLastNode();
            if (lastNode)
                m_headNodeId = lastNode->getId();
        }
    }
    else
        Q_ASSERT(false);
}

void CDirectionalWay::reverse()
{
    //change direction
    if (m_direction == forward)
        m_direction = backward;
    else
        m_direction = forward;

    //Swap head and tail nodes
    quint64 tid = m_headNodeId;
    m_headNodeId = m_tailNodeId;
    m_tailNodeId = tid;
}

void CDirectionalWay::getNodeList(NodeList& nodes)
{
    //Clear node list of any existing pointers 
    nodes.clear();
    NodeList wayNodes = m_way->getNodeList();

    //Copy the nodes for the way in the correct order based on direction 
    if (m_direction == forward)
    {
        for (int i = 0; i < wayNodes.size(); i++)
        {
            nodes.append(wayNodes.at(i));
        }
    }
    else
    {
        for (int i = 0; i < wayNodes.size(); i++)
        {
            nodes.append(wayNodes.at(wayNodes.size() - 1 - i));
        }
    }
}

