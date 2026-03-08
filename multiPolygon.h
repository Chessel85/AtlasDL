//MultiPolygon.h
#pragma once

#include <QObject>
#include <list>

class CNode;
class CWay;
class CPolygon;
class CDirectionalWay;

class CMultiPolygon : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CMultiPolygon(QObject* parent = nullptr);
    CMultiPolygon(const CMultiPolygon& original);
	~CMultiPolygon();

//Methods
    public:
        CPolygon* createPolygon( CWay* headWay );
        CPolygon* createPolygon(CDirectionalWay* dw );
        CPolygon* getLastPolygon();
        void moveToFirstPolygon();
        CPolygon* getNextPolygon();
        void addPolygon(CPolygon* polygon);
        void removeInnerRing(CPolygon* inner);
        bool isEmpty() const;
        int size() const;
        void enforceAnticlockwiseRotation();
        void joinLineStrings();
        void dumpInfo();
        void dumpInfo2();
        bool splitPolygonByLineStrings(CPolygon* polygon, CMultiPolygon& newLandPolygons, CMultiPolygon& offcuts);
        int numPolygons() const;

private:

//Member variables
private:
    std::list<CPolygon*> m_polygons;
    std::list<CPolygon*>::iterator m_polygonIterator;
};