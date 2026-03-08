//BoundingBox.h
#pragma once

#include <QString>
#include <QGeoCoordinate>

class CBoundingBox
{
//Constructor 
public:
    CBoundingBox();
    CBoundingBox(const CBoundingBox& bb);
    ~CBoundingBox();

//Methods
public:
    bool IsNull();
    void setBoundingBox(double s, double w, double n, double e);
    bool setBoundingBoxFromString( const QString& boundingBox );
    void reset();
    bool PointInBox(double x, double y);
    void Grow(double x, double y);
    void Grow(const CBoundingBox& rBB);
    std::string GetBoundingBoxAsString();
    double south();
    double west();
    double north();
    double east();
    double height();
    double width();
    QGeoCoordinate getCentre();
    void getCentre(double& x, double& y);
    bool Overlap( CBoundingBox& rBB2 );
    bool Contains(CBoundingBox& rBB2);

//Attributes
private:
    double m_south;
    double m_west;
    double m_north;
    double m_east;
    bool m_isNull;
};

