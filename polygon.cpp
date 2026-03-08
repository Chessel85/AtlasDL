//polygon.cpp

#include "polygon.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "way.h"
#include "directionalWay.h"
#include "BoundingBox.h"
#include "multiPolygon.h"
#include <QGeoCoordinate>
#include "geoToLambert.h"
#include <algorithm>

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(polygonTracker , "polygon.tracker")

//Memory management is that CPolygon is responsible for creating and destroying directional ways
//A directional way cannot be shared between two polygons 

CPolygon::CPolygon()
{
}


CPolygon::CPolygon( CWay* headWay, enumDirection direction )
{
    this->createAndAppendDirectionalWay(headWay, direction);
}

CPolygon::CPolygon(QList<PolygonWayEntry>& polygonWayList, WayMap& waysMaster )
{
    QList<CDirectionalWay*>* current = nullptr;
    for (PolygonWayEntry pwe : polygonWayList)
    {
        //Which polygon will the directional way go to
        if (pwe.sequenceNumber == 1 && pwe.innerRingId == 0)
            current = &m_polygon;
        else if (pwe.sequenceNumber == 1 && pwe.innerRingId > 0)
        {
            CPolygon* p = new CPolygon();
            m_internalRings.append(p );
            current = &(p->m_polygon);
        }

        CWay* way = waysMaster.value(pwe.wayId );
        CDirectionalWay* dw = new CDirectionalWay(pwe, way );
        current->append(dw);
        CBoundingBox bb;
        bb.setBoundingBox(pwe.south, pwe.west, pwe.north, pwe.east);
        m_boundingBox.Grow(bb );
    }
}

CPolygon::CPolygon(CPolygon* p)
{
    //Duplicate the polygon directional ways into this 
    for (CDirectionalWay* dw : p->m_polygon)
    {
        CDirectionalWay* ndw = new CDirectionalWay(dw);
        m_polygon.append(ndw);
    }

    //Copy any inner rings
    for (CPolygon* innerRing : p->m_internalRings)
    {
        CPolygon* newRing(innerRing);
        m_internalRings.append(newRing );
    }

    //Set bounding box
    m_boundingBox = p->m_boundingBox;
}

CPolygon::~CPolygon()
{
    destroyPolygon();
}

quint64 CPolygon::getTailNodeId()
{
    quint64 nodeId = 0;
    if (m_polygon.isEmpty() )
        return nodeId;

    //Get last directional way from the current polygon 
    CDirectionalWay* directionalWay = m_polygon.last();

    //The tail node accounts for direction of the way 
    nodeId = directionalWay->getTailNodeId();

    return nodeId;
}

CNode* CPolygon::getHeadNode()
{
    CNode* node = nullptr;
    if (m_polygon.isEmpty())
        return node;

    //Get first directional way from the current polygon 
    CDirectionalWay* directionalWay = m_polygon.first();

    //The head node accounts for direction of the way 
    node = directionalWay->getHeadNode();

    return node;
}

CNode* CPolygon::getTailNode()
{
    CNode* node = nullptr;
    if (m_polygon.isEmpty())
        return node;

    //Get last directional way from the current polygon 
    CDirectionalWay* directionalWay = m_polygon.last();

    //The tail node accounts for direction of the way 
    node = directionalWay->getTailNode();

    return node;
}

quint64 CPolygon::getHeadNodeId()
{
    quint64 nodeId = 0;
    if (m_polygon.isEmpty())
        return nodeId;

    //Get first directional way from the current polygon 
    CDirectionalWay* directionalWay = m_polygon.first();

    //The head node accounts for direction of the way 
    nodeId = directionalWay->getHeadNodeId();

    return nodeId;
}

bool CPolygon::isClosed() const
{
    if (m_polygon.isEmpty())
        return false;

    //Get first and last  directional ways
    CDirectionalWay* firstDirectionalWay = m_polygon.first();
    CDirectionalWay* lastDirectionalWay = m_polygon.last();

    quint64 headNodeId = firstDirectionalWay->getHeadNodeId();
    quint64 tailNodeId = lastDirectionalWay->getTailNodeId();

    bool isClosed = headNodeId == tailNodeId;
    return isClosed;
}

bool CPolygon::isValid(bool isClosed)
{
    //Polygon is valid if every directional way tail node is the same as the next head node 
    //And if closed flag is true means also makes a loop else is otherwise allowed to be a line string 

    //An empty polygon is valid
    if (m_polygon.isEmpty())
        return true;

    bool isValid = true;
    CDirectionalWay* dw2 = m_polygon.at(0);
    for (int i = 1; i < m_polygon.size(); i++)
    {
        CDirectionalWay* dw1 = dw2;
        dw2 = m_polygon.at(i);

        if (dw1->getTailNodeId() != dw2->getHeadNodeId())
        {
            isValid = false;
            break;
        }
    }

    //If valid at this point and expecting a closed polygon do final check
    if (isValid && isClosed)
    {
        CDirectionalWay* headDw = m_polygon.at(0);
        CDirectionalWay* tailDw = m_polygon.at(m_polygon.size() - 1);
        if (tailDw->getTailNodeId() != headDw->getHeadNodeId())
            isValid = false;
    }

    return isValid;
}

void CPolygon::appendPolygon(CPolygon* p)
{
    //Duplicate the directional ways to the end 
    for (CDirectionalWay* dw : p->m_polygon)
    {
        this->appendDirectionalWay(dw);
    }
}

void CPolygon::appendDirectionalWay(CDirectionalWay* dw, bool reverse )
{
    //Duplicate the directional way and add it 
    //Have kept memory allocation for directional ways to a minimum and this is one of the valid places 
    CDirectionalWay* ndw = new CDirectionalWay( dw );

    //If reverse flag is true then reverse the new directional way before appending
    if (reverse)
        ndw->reverse();

    m_polygon.append(ndw);
    m_boundingBox.Grow(ndw->getWay()->getBoundingBox());
}

void CPolygon::prependPolygon(CPolygon* p)
{
    //Create copy of incoming polygon  and then append this polygon to it
    // Then clear this and copy back the combined polygon 
    CPolygon combined;
    combined.appendPolygon(p );

    //Add on this list
    combined.appendPolygon(this );

    //Clear the this list
    this->destroyPolygon();

    //Copy across the combined list
    appendPolygon(&combined);

    //Clear the combined polygon
    combined.destroyPolygon();
}

void CPolygon::growPolygon(CWay* nextWay)
{
    CDirectionalWay* tailDirectionalWay = m_polygon.last();

    //Create a new directional way but determine direction for it based on tail way and shared node 
    enumDirection direction;
    quint64 tailNodeId = tailDirectionalWay->getTailNodeId();
    if (tailNodeId == nextWay->getFirstNode()->getId() )
        direction = forward;
    else
        direction = backward;

    //Can create the new directional way now
    this->createAndAppendDirectionalWay(nextWay, direction);
}

bool CPolygon::containsMBR(CPolygon* innerPolygon)
{
    bool inside = false;

    if (m_boundingBox.Contains(innerPolygon->m_boundingBox ) )
        inside = true;

        return inside;
}

QString CPolygon::getWayIdListAsText()
{
    QString values = "(";

    //Loop through directional ways to get the way id
    for (CDirectionalWay* dw : m_polygon)
    {
        QString wayId = QString::number(dw->getWayId());
        values += wayId + ",";
    }

    //Do the same for each inner ring
    for (CPolygon* inner : m_internalRings)
    {
        for( CDirectionalWay* dw : inner->m_polygon  )
        {
            QString wayId = QString::number(dw->getWayId());
            values += wayId + ",";
        }
    }

    //Replace trailing , with a closed bracket
    values.replace(values.size()-1, 1, ')');

    return values;
}

void CPolygon::addInnerRing(CPolygon* inner)
{
    m_internalRings.append(inner);
}

quint64 CPolygon::getHeadWayId()
{
    CDirectionalWay* dw = m_polygon.first();
    return dw->getWayId();
}

void CPolygon::moveToFirstDirectionalWay()
{
    m_directionalWayIterator = m_polygon.begin();
}

CDirectionalWay* CPolygon::getNextDirectionalWay()
{
    CDirectionalWay* dw = nullptr;

    if (m_directionalWayIterator != m_polygon.end())
    {
        dw = *m_directionalWayIterator;
        m_directionalWayIterator++;
    }

    return dw;
}

int CPolygon::getNumInnerRings()
{
    return m_internalRings.size();
}

int CPolygon::numWays()
{
    return m_polygon.size();
}

CPolygon* CPolygon::getInnerRing(int ringIndex)
{
    return m_internalRings.at(ringIndex);
}

CDirectionalWay* CPolygon::getInternalDirectionalWayFromNodeId(quint64 headNodeId)
{
    //Loop through internal rings looking for the head node
    for (CPolygon* ir : m_internalRings)
    {
        //Loop through the directional ways of the internal ring
        for (CDirectionalWay* idw : m_polygon)
        {
            if (idw->getHeadNodeId() == headNodeId)
                return idw;
        }
    }

    return nullptr;
}

bool CPolygon::enforceAnticlockwise()
{
    //check there are directional ways
    if (m_polygon.isEmpty())
        return true;

    //This method depends on having real way and node objects to point at so fail if not the case
    CDirectionalWay* dw = m_polygon.first();
    if (dw->getWay() == nullptr)
        return false;

        //First check the perimeter polygon and enforce direction and then go through inner rings
    bool perimeterClockwise =  isPolygonClockwise(this);
    if (perimeterClockwise)
        this->reverse();

    //Iterate through internal rings and ensure are clockwise 
    for (CPolygon* polygon : m_internalRings)
    {
        bool internalRingClockwise = isPolygonClockwise(polygon);
        if (!internalRingClockwise)
            polygon->reverse();
    }

    return true;
}

void CPolygon::destroyPolygon()
{
    //Delete all the CDirectionalWay objects 
    for (CDirectionalWay* dw : m_polygon)
    {
        delete dw;
    }
    m_polygon.clear();

    //And the internal polygons
    for (CPolygon* ip : m_internalRings)
    {
        delete ip;
    }
    m_internalRings.clear();
}

void CPolygon::createAndAppendDirectionalWay(CWay* headWay, enumDirection direction)
{
    CDirectionalWay* dw = new CDirectionalWay(headWay, direction);
    m_polygon.append(dw);
    m_boundingBox.Grow(headWay->getBoundingBox());
}

bool CPolygon::isPolygonClockwise(CPolygon* polygon)
{
    double signedArea = 0.0;
    QPointF p1, p2;

    //Need to compile a list of all the nodes in the polygon 
    NodeList polygonNodes;
    // Get the head node of the polygon to make further looping simpler 
    CNode* headNode = polygon->getHeadNode();

    //A polygon may or may not contain pointers to real nodes and way so abort if not
    if( !headNode )
    {
    Q_ASSERT( false );
    return false;
    }
    polygonNodes.append(headNode);

    //Loop through directional ways 
    for (CDirectionalWay* dw : polygon->m_polygon)
    {
        //The directional way returns node list correctly orientated 
        NodeList dwNodes;
        dw->getNodeList(dwNodes );

        // Add nodes to the main polygon sequence skipping the head node of each subsequent way to avoid duplicates
        for (int i = 1; i < dwNodes.size(); i++)
            polygonNodes.append(dwNodes.at(i));
    }

    //Create a transformer to a planar  coordinate system
    CGeoToLambert transformer;
    QGeoCoordinate centre = polygon->m_boundingBox.getCentre();
    transformer.CreateProjection(centre);

    // Now, apply the shoelace formula on the ordered list of polygon nodes
    int n = polygonNodes.size() - 1;
    for (int i = 0; i < n; i++)
    {
        CNode* node1 = polygonNodes.at(i);
        QGeoCoordinate geoPoint1 = node1->getCoordinate();
        transformer.transformPoint(geoPoint1, p1);

        CNode* node2 = polygonNodes.at(i + 1);
        QGeoCoordinate geoPoint2 = node2->getCoordinate();
        transformer.transformPoint(geoPoint2, p2);

        //Contribute these points to the area calculation 
        //Convert from square metres to square kilometres by divining by one million 
        signedArea += ((p1.x() * p2.y()) - (p2.x() * p1.y())) / 1000000.0;
    }

    signedArea /= 2.0;

    return signedArea < 0;
}

void CPolygon::reverse()
{
    //This means reverseing the order of the entries in the QList plus changing the direction of each way
    std::reverse(m_polygon.begin(), m_polygon.end());

    //Now iterate through list to reverse each directional way
    for (CDirectionalWay* dw : m_polygon)
    {
        dw->reverse();
    }
}

bool CPolygon::splitByLineString(CPolygon* linestring, CMultiPolygon& newLandPolygons, CMultiPolygon& newWaterPolygons, QString& errorMessage)
{
    /*
    //All polygons follow anticlockwise rule so follow the polygon (this)round in anticlockwise direction until find where linestring tail intersects
    bool found = false;
    quint64 linestringFirstNodeId = linestring->getHeadNodeId();
    quint64 linestringLastNodeId = linestring->getTailNodeId();
    CNode* linestringFirstNode =        linestring->getHeadNode();
    CNode* linestringLastNode = linestring->getTailNode();
    qCInfo(polygonTracker) << "Coastline head coordinates:" << linestringFirstNode->getLongitude() << "," << linestringFirstNode->getLatitude();
    qCInfo(polygonTracker) << "Coastline tail coordinates:" << linestringLastNode->getLongitude() << "," << linestringLastNode->getLatitude();

    //Start iterating around this country polygon until find intersection with tail of coastline line string 
    qCInfo(polygonTracker) << "Looping for coastline-tail intersection with country polygon.";
    QList<CDirectionalWay*> ::iterator it = m_polygon.begin();
    for (int i = 0; i < m_polygon.size(); i++)
    {
        CDirectionalWay* dw = *it;
        CNode* headNode = dw->getHeadNode();
        double distance = headNode->distance(linestringLastNode);
        qCInfo(polygonTracker) << "Distance" << distance << "at point" << i << "seeking coastline tail.";
        if (dw->getHeadNodeId() == linestringLastNodeId)
        {
            qCInfo(polygonTracker) << "Found coastline-tail.";
            found = true;
            break;
        }
        it++;
    }

    if (!found)
    {
        errorMessage = "Failed to find initial intersection of polygon and tail of coastline.";
        return false;
    }

    //Continue  iterating through the country list appending entries to a newly created land polygon 
    CPolygon* land = new CPolygon;
    qCInfo(polygonTracker) << "Looping for coastline-head intersection with country polygon.";
    found = false; //Looking for start of coastline 
    for (int i = 0; i < m_polygon.size(); i++)
    {
        CDirectionalWay* dw = *it;
        land->appendDirectionalWay(dw); 
        CNode* tailNode = dw->getTailNode();
        double distance = tailNode->distance(linestringFirstNode);
        qCInfo(polygonTracker) << "Distance" << distance << "at point" << i << "seeking coastline head.";
        if (dw->getTailNodeId() == linestringFirstNodeId)
        {
            qCInfo(polygonTracker) << "Found coastline-head.";
            found = true;
            break;
        }
        it++;
        if (it == m_polygon.end())
            it = m_polygon.begin();
    }

    if (!found)
    {
        errorMessage = "Failed to find where country polygon finds head of coastline.";
        delete land;
        return false;
    }

    //Finished going round the country so close the polygon by adding on the coastline
    qCInfo(polygonTracker) << "Closing trimmed polygon with coastline.";
    for (CDirectionalWay* dw : linestring->m_polygon)
    {
        land->appendDirectionalWay(dw); //Land is only a temporary container of poinnters so do not need to allocate memory 
    }

    //This is the end of the land polygon which is now closed
    Q_ASSERT(land->isClosed());
    newLandPolygons.addPolygon(land);

    //Conveniently the iterator is at the right place to start the territorial water offcut 
    qCInfo(polygonTracker) << "Searching territorial water intersection for coastline-tail.";
    found = false;
    it++;
    if (it == m_polygon.end())
        it = m_polygon.begin();
    for (int i = 0; i < m_polygon.size(); i++)
    {
        CDirectionalWay* dw = *it;
        //Need to create offcut if not done so already 
        if (!offcut)
            offcut = new CPolygon();
        offcut->appendDirectionalWay(dw);
        if (dw->getTailNodeId() == linestringLastNodeId)
        {
            found = true;
            break;
        }
        it++;
        if (it == m_polygon.end())
            it = m_polygon.begin();
    }

    if (!found)
    {
        errorMessage = "Failed to match new territorial water polygon to tail of coastline.";
        return false;
    }

    //Add on the coastline ways to the offcut this time going backward  to maintain anticlockwise rule 
    qCInfo(polygonTracker) << "Closing territorial water polygon with reverse coastline.";
    for (QList<CDirectionalWay*>::reverse_iterator rit = linestring->m_polygon.rbegin(); rit != linestring->m_polygon.rend(); rit++)
    {
        CDirectionalWay* lsdw = *rit;
        offcut->appendDirectionalWay(lsdw, true); //The true means reverse the directional way before appending 
    }

    //Land list is now complete and all processing of this polygon is done so replace this polygon with it 
    this->destroyPolygon();

    //Copy land polygon to this polygon 
    appendPolygon(&land);
    land.destroyPolygon();
*/
    return true;
}

CPolygon* CPolygon::extractSection(quint64 startNodeId, quint64 endNodeId, bool reverse )  
{
    //reverse parameter defaults to false but when true go other way around the polygon reversing each dw en route 
    CPolygon* p = new CPolygon;

    if (!reverse)
    {
        //Loop until find dw with head node matching start node
        int i = 0;
        for (i = 0; i < m_polygon.size(); i++)
        {
            CDirectionalWay* dw = m_polygon.at(i);
            if (dw->getHeadNodeId() == startNodeId)
                break;
        }
        Q_ASSERT(i < m_polygon.size());

        ////Continue looping and adding the dw to the new polygon
        int counter = 0;
        while (counter < m_polygon.size())
        {
            CDirectionalWay* dw = m_polygon.at(i);
            p->appendDirectionalWay(dw);

            //Check if have reached the end
            if (dw->getTailNodeId() == endNodeId)
                break;

            //Incremement counters and reset i back to start of array if needed 
            i++;
            counter++;
            if (i == m_polygon.size())
                i = 0;
        }
    }
    else
    {
        //Reverse is true so do everything the other way around 
        //Loop until find dw with head node matching end node
        int i = 0;
        for (i = m_polygon.size()-1; i > -1; i--)
        {
            CDirectionalWay* dw = m_polygon.at(i);
            if (dw->getTailNodeId() == startNodeId)
                break;
        }
        Q_ASSERT(i < m_polygon.size());

        ////Continue looping and adding the dw to the new polygon
        int counter = 0;
        while (counter < m_polygon.size())
        {
            CDirectionalWay* dw = m_polygon.at(i);
            dw->reverse();
            p->appendDirectionalWay(dw);

            //Check if have reached the end
            if (dw->getHeadNodeId() == endNodeId)
                break;

            //Incremement counters and reset i back to start of array if needed 
            i--;
            counter++;
            if (i == -1 )
                i = m_polygon.size()-1;
        }
    }

    Q_ASSERT(!p->m_polygon.isEmpty());

    return p;
}

bool CPolygon::containsNode(quint64 nodeId)
{
    bool contains = false;

    for (CDirectionalWay* dw : m_polygon)
    {
        if (dw->getHeadNodeId() == nodeId || dw->getTailNodeId() == nodeId)
        {
            contains = true;
            break;
        }
    }

    return contains;
}