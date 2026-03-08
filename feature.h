//Feature.h
#pragma once

#include <QObject>
#include "multiPolygon.h"
#include "polygonWayEntry.h"

class CRelation;
class CWay;
class CNode;


class CFeature : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CFeature(CRelation* relation, QObject* parent = nullptr);
	~CFeature();

//Methods
    public:
        bool isEmpty();
        bool deducePolygons();
        CMultiPolygon& getPolygons();
        CMultiPolygon& getInnerRings();
        CMultiPolygon& getLinestrings();

private:

//Member variables
private:
    CMultiPolygon m_multipolygon;
    CMultiPolygon m_innerPolygons;
    CMultiPolygon m_lineStrings;
    CRelation* m_relation;
};