//MultiPolygon.cpp

#include "MultiPolygon.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "way.h"
#include "directionalWay.h"
#include "polygon.h"


//Set up logging category hierarchy
Q_LOGGING_CATEGORY(multipolygonTracker, "multipolygon.tracker")


CMultiPolygon::CMultiPolygon( QObject* parent )
    : QObject( parent) 
{
}

CMultiPolygon::CMultiPolygon(const CMultiPolygon& original)
{
    //Duplicate contents from original to this
    for (CPolygon* p : original.m_polygons)
    {
        CPolygon* newPolygon(p);
        m_polygons.push_back(newPolygon);
    }
}

CMultiPolygon::~CMultiPolygon()
{
    //Delete all the polygons 
    for (CPolygon*polygon : m_polygons )
    {
        delete polygon;
    }
    m_polygons.clear();
    }

CPolygon* CMultiPolygon::createPolygon(CWay* headWay)
{
    //Create a polygon keyed on the first way id 
    CPolygon* polygon = new CPolygon(headWay, forward);
    m_polygons.push_back(polygon);
    return polygon;
    }

CPolygon* CMultiPolygon::createPolygon(CDirectionalWay* dw)
{
    //Create a polygon keyed on the directional way 
    CPolygon* p = new CPolygon();
    p->appendDirectionalWay(dw);
    m_polygons.push_back(p );
    return p;
}


CPolygon* CMultiPolygon::getLastPolygon()
{
    CPolygon* polygon = nullptr;
    if (m_polygons.size() == 0  )
        return nullptr;

    //Get last polygon from the main list 
    polygon = m_polygons.back();

    return polygon;
}

void CMultiPolygon::moveToFirstPolygon() 
{
    m_polygonIterator = m_polygons.begin();
}

CPolygon* CMultiPolygon::getNextPolygon()
{
    CPolygon* polygon = nullptr;

    if (m_polygonIterator != m_polygons.end() )
    {
        polygon = *m_polygonIterator;
        m_polygonIterator++;
    }

    return polygon;
}

void CMultiPolygon::addPolygon(CPolygon* polygon)
{
    m_polygons.push_back( polygon);
}

void CMultiPolygon::removeInnerRing(CPolygon* inner )
{
    auto it = m_polygons.begin();
    while( it != m_polygons.end() )
    {
        CPolygon* polygon = *it;
        quint64 wayId = polygon->getHeadWayId();
    quint64 innerWayId = inner->getHeadWayId();
    if (wayId == innerWayId)
    {
        m_polygons.erase(it);
        break;
    }
    it++;
    }
}

bool CMultiPolygon::isEmpty() const
{
    return (m_polygons.size() == 0);
}

int CMultiPolygon::size()  const
{
    return m_polygons.size();
}

void CMultiPolygon::enforceAnticlockwiseRotation()
{
    //For each polygon enforce perimeter anticlockwise and internal rings clockwise 
    for (CPolygon* polygon : m_polygons)
    {
        polygon->enforceAnticlockwise();
    }
}

void CMultiPolygon::joinLineStrings()
{
    //Only have work to do if there are more than one line strings
    if (m_polygons.size() <= 1)
        return;

    //Work through collection of unclosed polygons and see if can join them together into longer string
        // Start with an iterator to the beginning of the map
    auto itOuter = m_polygons.begin();

    // Outer loop 
    while( itOuter != m_polygons.end() )
    {
        CPolygon* outerPolygon = *itOuter;
        Q_ASSERT(!outerPolygon->isClosed());
        quint64 outerHeadNodeId = outerPolygon->getHeadNodeId();
        quint64 outerTailNodeId = outerPolygon->getTailNodeId();

// Inner loop iterates through remainder of list 
        //Actually have to start at beginning again because an inner line string may have merged with a later line string 
        auto itInner = m_polygons.begin(); //itOuter;

        while( itInner != m_polygons.end() )
        {
            //If inner and outer are the same can skip 
            if (itOuter == itInner)
            {
                itInner++;
                continue;
            }

            CPolygon* innerPolygon = *itInner;
            Q_ASSERT(!innerPolygon->isClosed());
            quint64 innerTailNodeId = innerPolygon->getTailNodeId();
            quint64 innerHeadNodeId = innerPolygon->getHeadNodeId();

            //Check if inner head joins to outer tail 
            if (outerTailNodeId == innerHeadNodeId)
            {
                outerPolygon->appendPolygon(innerPolygon);
                delete innerPolygon;
                itInner = m_polygons.erase(itInner);
                outerTailNodeId = outerPolygon->getTailNodeId();
            }
            else if(outerHeadNodeId == innerTailNodeId )
            {
                outerPolygon->prependPolygon(innerPolygon);
                delete innerPolygon;
                itInner = m_polygons.erase(itInner);
                outerHeadNodeId = outerPolygon->getHeadNodeId();
            }
            else 
            {
                // No join so move to the next inner segment
                itInner++;
            }
        }
        itOuter++;
    }
}


void CMultiPolygon::dumpInfo2()
{
    int counter = 1;
    for (CPolygon* p : m_polygons)
    {
        CNode* headNode = p->getHeadNode();
        CNode* tailNode = p->getTailNode();
        qDebug() << "Line string" << counter++ << ": Head x:" << headNode->getLongitude() << "Head y:" << headNode->getLatitude();
        qDebug() << ": Tail x:" << tailNode->getLongitude() << "Head y:" << tailNode->getLatitude();
            }
}

void CMultiPolygon::dumpInfo()
{
    int t = 0;
    QString num = "";
    for (CPolygon* p : m_polygons)
    {
        int n = p->numWays();
        t += n;
        num += QString::number(n) + ","; ;
    }
    qDebug() << num;
}

bool CMultiPolygon::splitPolygonByLineStrings(CPolygon* polygon, CMultiPolygon& newLandPolygons, CMultiPolygon& newWaterPolygons)
{
    //This object holds the linestrings to split polygon by 
    //Must preserve the original polygon and add newly formed land and territorial water polygons to respective collections 
    bool ok = true;
    bool oneError = false;
    int counter = 1; //debug code 
    //Go through line strings and split the passed in polygon into new land and water polygons 
    for (CPolygon* linestring : m_polygons)
    {
        QString errorMessage;
        Q_ASSERT(linestring->isValid( false ));
        qCInfo(multipolygonTracker) << "Trimming with coastline line string" << counter++;
        ok = polygon->splitByLineString(linestring, newLandPolygons, newWaterPolygons, errorMessage);
        if (!ok)
        {
            oneError = true;
            qCInfo(multipolygonTracker) << errorMessage;
            continue;
        }
    }
    return oneError;
}

int CMultiPolygon::numPolygons() const 
{
    return m_polygons.size();
}