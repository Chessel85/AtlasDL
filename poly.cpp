//poly.cpp

#include "poly.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "way.h"
#include "directionalWay.h"
#include "path.h"
#include "paths.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(polyTracker, "poly.tracker")

//Memory management is that CPoly is responsible for creating and destroying CPath objects 
//A CPath cannot be shared between two CPoly instances 

CPoly::CPoly()
{
}

CPoly::CPoly(CPath* closedPath)
{
    m_perimeter.addPath(closedPath);
}
CPoly::~CPoly()
{
    destroyPolygon();
}

void CPoly::destroyPolygon()
{
    //Perimeter is destroyed by going out of scope of this instance 

    //Delete inner rings
    for (CPath* innerRing : m_innerRings)
    {
        delete innerRing;
    }
    m_innerRings.clear();
}

bool CPoly::isClosed() const
{
    //Closed is when perimeter and all inner rings are closed
    bool closed = m_perimeter.isClosed();

    for (int i = 0; i < m_innerRings.size() && closed; i++)
    {
        closed = m_innerRings.at(i)->isClosed();
    }

    return closed;
}


bool CPoly::isValid() const
{
    //Valid means that every path is valid 
    bool isValid = m_perimeter.isValid();

    for (int i = 0; i < m_innerRings.size() && isValid; i++)
    {
        CPath* innerRing = m_innerRings.at(i);
        isValid = innerRing->isValid();
    }

    return isValid;
}

bool CPoly::buildFromWayList(PolygonWayList& polygonWayList, WayMap& waysMaster)
{
    CPath* path = nullptr;
    int index = 0;
    int ringCounter = 0;
    for (index = 0; index < polygonWayList.size(); index++)
    {
        PolygonWayEntry pwe = polygonWayList.at(index);
        if (pwe.innerRingId > 0)
            break;

        //Get the CWay pointer
        CWay* way = waysMaster.value(pwe.wayId);
        Q_ASSERT(way);

        m_perimeter.addWayEntry(pwe, way );
        m_boundingBox.Grow(way->getBoundingBox());
    }

    //Perimeter is finished so do any inner rings
    for( index; index < polygonWayList.size(); index++ )
    {
        PolygonWayEntry pwe = polygonWayList.at(index);

        //Get the CWay pointer
        CWay* way = waysMaster.value(pwe.wayId);
        Q_ASSERT(way);

        if (pwe.innerRingId > ringCounter )
        {
            path = new CPath;
            m_innerRings.append(path);
            ringCounter++;
        }
        Q_ASSERT(path);
        path->addWayEntry(pwe, way);
    }
    return true;
}

void CPoly::getIntersections(CPath* path, QVector<quint64>& anchors)
{
    //Iterate around the perimeter and the inner rings to see if directional way head nodes match head or tail of the path 
    m_perimeter.getIntersections(path, anchors);

    for (CPath* innerRing : m_innerRings)
    {
        innerRing->getIntersections(path, anchors);
    }
}

bool CPoly::getDirectionalWayWithHeadNodeId(quint64 anchorNodeId, CDirectionalWay*& dw, int& ring )
{
    //Locate this head node on perimeter or inner rings
    dw = m_perimeter.getDirectionalWay(anchorNodeId);
    if (dw)
    {
        ring = 0;
        return true;
    }

    //Try inner rings
    int counter = 1;
    for (CPath* innerRing : m_innerRings)
    {
        dw = innerRing->getDirectionalWay(anchorNodeId);
        if (dw)
        {
            ring = counter;
            return true;
        }
        counter++;
    }
    //If get this far then have not found the directional way in perimeter or inner rings 
    return false;
}

CPath* CPoly::duplicateSection(quint64 startNodeId, int ring, QVector<quint64>& anchors)
{
    //Dirty code is to copy anchors into a QSet for quick lookup
    QSet<quint64> anchorSet;
    for (quint64 anchorId : anchors)
        anchorSet.insert(anchorId);

    //Identify a source ring which is either the perimeter or an inner ring
    CPath* sourceRing = nullptr;
    if (ring == 0)
        sourceRing = &m_perimeter;
    else
        sourceRing = m_innerRings.at(ring - 1);

    //Create new path to copy directional ways into 
    CPath* path = new CPath;

    quint64 headNodeId = startNodeId;
    CDirectionalWay* dw = nullptr;
    while ((dw = sourceRing->getDirectionalWay(headNodeId)))
    {
        path->addDirectionalWay(dw);
        if (anchors.contains(dw->getTailNodeId()))
            break;
        headNodeId = dw->getTailNodeId();
    }
    
    return path;
}

