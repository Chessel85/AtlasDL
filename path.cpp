//Path.cpp

#include "Path.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "way.h"
#include "directionalWay.h"
#include "polygonWayEntry.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(PathTracker , "Path.tracker")

//Memory management is that CPath is responsible for creating and destroying CDirectionalWay objects 
//A CDirectionalWay cannot be shared between two CPath instances 
//The forward and backward hash containers do share the same directional way  with m_DwByHead considered the primary container but no real difference between them 

CPath::CPath()
{
    m_headNodeId = 0;
    m_tailNodeId = 0;
}

CPath::~CPath()
{
    destroyDirectionalWays();
}

void CPath::destroyDirectionalWays()
{
    for (CDirectionalWay* dw : m_DwByHead)
    {
        delete dw;
    }
    m_DwByHead.clear();
    m_DwByTail.clear();
}

quint64 CPath::getHeadNodeId() const
{
    return m_headNodeId;
}

quint64 CPath::getTailNodeId() const
{
    return m_tailNodeId;
}
    
CNode* CPath::getHeadNode()
{
    CDirectionalWay* dw = m_DwByHead.value(m_headNodeId);

    CNode* node = nullptr;
    if (dw)
        node = dw->getHeadNode();

    return node;
}

CNode* CPath::getTailNode()
{
    CDirectionalWay* dw = m_DwByHead.value(m_tailNodeId);

    CNode* node = nullptr;
    if (dw)
        node = dw->getTailNode();

    return node;
}

bool CPath::isClosed() const
{
    //An empty path is not closed
    if (m_DwByHead.isEmpty() )
        return false;

    bool closed = m_headNodeId== m_tailNodeId;

    return closed;
}

bool CPath::isValid() const
{
    //Valid means each directional way tail node is the next head node 
    //And each directional way is used 

        //An empty path is not valid 
    if (m_DwByHead.isEmpty() )
        return false;

    quint64 headNodeId = m_headNodeId;
    int i = 0;
    for (i = 0; i < m_DwByHead.size(); i++)
    {
        CDirectionalWay* dw = m_DwByHead.value(headNodeId);

        //Escape condition 
        if (!dw)
            break;

        //Update head node for next time around the loop 
        headNodeId = dw->getTailNodeId();
    }

    bool isValid = (i == m_DwByHead.size());

    return isValid;
}

void CPath::addDirectionalWay(CDirectionalWay* dw)
{
    //Copy this directional way
    CDirectionalWay* ndw = new CDirectionalWay(dw);

    //if this is the first entry then this is the head node
    if (m_DwByHead.isEmpty())
        m_headNodeId = ndw->getHeadNodeId();

    //The tail node is always the last one
    m_tailNodeId = ndw->getTailNodeId();

    //Add the new directional way into the collections 
    Q_ASSERT(!m_DwByHead.value(ndw->getHeadNodeId(), nullptr));
    Q_ASSERT(!m_DwByTail.value(ndw->getTailNodeId(), nullptr));
    m_DwByHead.insert(ndw->getHeadNodeId(), ndw);
    m_DwByTail.insert(ndw->getTailNodeId(), ndw);

    //Grow the bounding box
    if( ndw->getWay() )
        m_boundingBox.Grow(ndw->getWay()->getBoundingBox());
}

void CPath::addWayEntry(PolygonWayEntry& pwe, CWay* way)
{
    CDirectionalWay* dw = new CDirectionalWay(pwe, way );
    addDirectionalWay(dw);
    delete dw;
}

void CPath::addPath(CPath* path)
{
    //Move through path starting at head node and add to this 
    //Incoming path could be a closed loop so only go round once 
    int counter = 0;
    CDirectionalWay* dw = nullptr;
    quint64 headNodeId = path->m_headNodeId;
    while( ( dw = path->m_DwByHead.value( headNodeId ) ) && counter < path->m_DwByHead.size() )
    {
        addDirectionalWay(dw);
        headNodeId = dw->getTailNodeId();
        counter++;
    }
    return;
}

void CPath::reverse()
{
    //Create a temporary container 
    QList<CDirectionalWay*> container;
    for (CDirectionalWay* dw : m_DwByHead)
    {
        dw->reverse();
        container.append(dw);
    }

    //Clear the main containers and repopulate
    m_DwByHead.clear();
    m_DwByTail.clear();
    for (CDirectionalWay* dw : container)
    {
        m_DwByHead.insert(dw->getHeadNodeId(), dw);
        m_DwByTail.insert(dw->getTailNodeId(), dw);
    }

    //Finally reverse start and end nodes
    quint64 nodeId = m_headNodeId;
    m_headNodeId = m_tailNodeId;
    m_tailNodeId = nodeId; 
    }

void CPath::getIntersections(CPath* path, QVector<quint64>& anchors)
{
    //Iterate through this path directional ways and see if head node matches head or tail node of passed in path
    quint64 pathHeadNodeId = path->getHeadNodeId();
    quint64 pathTailNodeId = path->getTailNodeId();

    for (CDirectionalWay* dw : m_DwByHead)
    {
        quint64 dwHeadNodeId = dw->getHeadNodeId();
        if (dwHeadNodeId == pathHeadNodeId || dwHeadNodeId == pathTailNodeId)
        {
            anchors.append(dwHeadNodeId);
            qCInfo(PathTracker) << "Anchor detected at" << dw->getHeadNode()->getLatitude() << ", " << dw->getHeadNode()->getLongitude();;
        }
    }
}

CDirectionalWay* CPath::getDirectionalWay(quint64 headNodeId)
{
    CDirectionalWay* dw = m_DwByHead.value(headNodeId, nullptr );

    return dw;
}

CDirectionalWay* CPath::getDirectionalWayByTail(quint64 tailNodeId)
{
    CDirectionalWay* dw = m_DwByTail.value(tailNodeId, nullptr);

    return dw;
}

QString CPath::getWayIdListAsText() const
{
    QString wayIdList;

    //Loop through the directional ways to compile a comma seperated list of way id
    for (CDirectionalWay* dw : m_DwByHead)
    {
        wayIdList += QString::number( dw->getWayId() ) + ",";
    }

    //Trim off the last  comma which is null string safe 
    wayIdList.chop(1);

    return wayIdList;
}

QString CPath::getWayList(int ringNumber) const
{
    //Build up bracketed and comma separated  list of way list info for inserting into database
    QString wayList;

    quint64 headNodeId = m_headNodeId;
    CDirectionalWay* dw = nullptr;
    int sequenceNumber = 0;

    while ((dw = m_DwByHead.value(headNodeId)) &&
        sequenceNumber < m_DwByHead.size() )
    {
        QString wayListRow = "(" + 
            QString("<polygonId>,") +
            QString::number( ringNumber) + "," +
            QString::number(sequenceNumber++) + "," +
            QString::number(dw->getDirection() ) + "," +
            QString::number(dw->getWayId() ) + "),";

        //Join onto main list 
        wayList += wayListRow;

        //Move onto next dw
        headNodeId = dw->getTailNodeId();
    }

    return wayList;
}

