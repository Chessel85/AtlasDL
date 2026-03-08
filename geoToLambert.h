//GeoToLambert
#pragma once

#include <QPolygonF> 
#include <QGeocoordinate>
#include <QPointF>
#include <proj.h>

class CGeoToLambert
{
// Constructor
public:
    CGeoToLambert();
    ~CGeoToLambert();

//Methods 
public:
    bool CreateProjection(const QGeoCoordinate& centre);

    // Method to transform a polygon from SRID 4326 to Lambert Azimuthal coordinates
    //bool  transform(const QGeoPolygon& geoPolygon, QPolygonF& screenPolygon );
    bool  transformPoint(const QGeoCoordinate& geoPoint, QPointF& planarPoint );

private:
    void CreateContext();
    void DestroyObjects();

    // Member variables
    PJ_CONTEXT* m_projContext;
    PJ* m_projTransform; 
        };
