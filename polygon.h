//polygon.h
#pragma once

#include <QList>
#include "directionalWay.h"
#include "BoundingBox.h"
#include "PolygonWayEntry.h"

class CNode;
class CWay;
class CDirectionalWay;
class CPolygon;
class CMultiPolygon;
class QGeoCoordinate;

class CPolygon 
{
//Constructor
public:
    explicit CPolygon();
    explicit CPolygon( CWay* headWay, enumDirection direction  );
    explicit CPolygon(QList<PolygonWayEntry>& polygonWayList, WayMap& waysMaster );
    explicit CPolygon(CPolygon* p);
	~CPolygon();

    public:
        CNode* getHeadNode();
        CNode* getTailNode();
        quint64 getHeadNodeId();
        quint64 getTailNodeId();
        quint64 getHeadWayId();
        bool isClosed() const;
        bool isValid(bool isClosed = true );
        void appendPolygon(CPolygon* p);
        void appendDirectionalWay(CDirectionalWay* dw, bool reverse = false );
        void prependPolygon(CPolygon* p);
        void growPolygon(CWay* nextWay );
        bool containsMBR(CPolygon* innerPolygon );
        void addInnerRing(CPolygon* inner);
        void moveToFirstDirectionalWay();
        CDirectionalWay* getNextDirectionalWay();
        int getNumInnerRings();
        int numWays();
        CPolygon* getInnerRing( int ringIndex );
        CDirectionalWay* getInternalDirectionalWayFromNodeId(quint64 headNodeId);
        bool enforceAnticlockwise();
        bool splitByLineString(CPolygon* lineString, CMultiPolygon& newLandPolygons, CMultiPolygon& newWaterPolygons, QString& errorMessage );
        QString getWayIdListAsText();
        CPolygon* extractSection(quint64 startNodeId, quint64 endNodeId, bool reverse = false );
        bool containsNode(quint64 nodeId);


private:
    void destroyPolygon();
    void createAndAppendDirectionalWay(CWay* headWay, enumDirection direction);
    //QString getPolygonWKT(CPolygon* polygon );
    bool isPolygonClockwise( CPolygon* polygon );
    void reverse();

//Member variables
private:
    QList<CDirectionalWay*> m_polygon;
    QList<CPolygon*> m_internalRings;
    CBoundingBox m_boundingBox;
    QList<CDirectionalWay*>::iterator m_directionalWayIterator;
};