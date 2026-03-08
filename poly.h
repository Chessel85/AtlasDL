//poly.h
#pragma once

#include <QHash>
#include "path.h"
#include "BoundingBox.h"
#include "polygonWayEntry.h"
#include "using.h"

class CNode;
class CWay;
class CDirectionalWay;
class CPaths;
class CSpatial;

class CPoly 
{
//Constructor
public:
    explicit CPoly();
    explicit CPoly(CPath* closedPath );
    ~CPoly();

    public:
    bool isClosed() const;
    bool isValid() const;
    bool buildFromWayList(PolygonWayList& polygonWayList, WayMap& waysMaster);
    QString getWayIdListAsText();
    QString getWayListAsText();
    void getIntersections(CPath* path, QVector<quint64>& anchors);
    bool getDirectionalWayWithHeadNodeId(quint64 anchorNodeId, CDirectionalWay*& dw, int& ring );
    CPath* duplicateSection(quint64 headNodeId, int ring, QVector<quint64>& anchors);

private:
    void destroyPolygon();

//Member variables
private:
    CPath m_perimeter;
    QList<CPath*> m_innerRings;
    CBoundingBox m_boundingBox;
};