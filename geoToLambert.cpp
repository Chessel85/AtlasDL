//GeoToLambert.cpp

#include "GeoToLambert.h"
#include <iostream>
#include <QPointF>
#include <vector>
#include <QDebug>

CGeoToLambert::CGeoToLambert()
{
    //Create context 
    CreateContext();
}

CGeoToLambert::~CGeoToLambert()
{
    DestroyObjects();
}

void CGeoToLambert::CreateContext()
{
    m_projContext = proj_context_create();
    if (!m_projContext)
    {
        qDebug() << "CGeoToLambert: Failed to create context.";
    }
}

void CGeoToLambert::DestroyObjects()
{
    // Clean up the proj objects to prevent memory leaks
    if (m_projTransform)
    {
        proj_destroy(m_projTransform);
    }

    if (m_projContext)
    {
        proj_context_destroy(m_projContext);
    }
}

bool CGeoToLambert::CreateProjection(const QGeoCoordinate& centre)
{
    //Check a context is in place
    if (!m_projContext)
    {
        qDebug() << "CGeoToLambert: No context available when creating projections.";
        return false;
    }

    // Construct the PROJ string with the given center coordinates
    PJ_COORD  pjCentre = proj_coord(centre.longitude(), centre.latitude(), 0, 0);
    std::string projString = "+proj=laea +lat_0=" + std::to_string(pjCentre.xyz.y ) + " +lon_0=" + std::to_string(pjCentre.xyz.x ) + " +ellps=WGS84 +units=m";

    //Create an interim projection transformation
    PJ* p = proj_create_crs_to_crs(m_projContext, "EPSG:4326", projString.c_str(), nullptr);
    if (p == nullptr)
    {
        qDebug() << "CGeoToLambert: Failed to create PROJ transformation pipeline.";
        int err = proj_context_errno(m_projContext);
        qDebug() << "PROJ error code: " << err << " (" << proj_errno_string(err) << ")";
        return false;
    }

    // Normalize for visualization to get lon/lat order and use degrees not radians 
    m_projTransform = proj_normalize_for_visualization(m_projContext, p );
    if (m_projTransform == nullptr)
    {
        qDebug() << "CGeoToLambert: Failed to normalize PROJ transformation.";
        proj_destroy(p );
        return false;
    }
    //Tidy up  the interim projection 
    proj_destroy(p);

    return true;
}
/*bool CGeoToLambert::transform(const QGeoPolygon& geoPolygon, QPolygonF& screenPolygon)
{
    // Ensure the transformation object has been created
    if (!m_projTransform) 
    {
        qWarning() << "CGeoToLambert: No projection created. Call CreateProjection first.";
        screenPolygon = QPolygonF();
        return false;
    }

    // The number of points in the input polygon
    int pointCount = geoPolygon.size();
    if (pointCount == 0) 
    {
        screenPolygon.clear();
        return 0; // Nothing to do
    }

    // Create vectors to hold coordinates which deals with memory management 
    std::vector<double> lons(pointCount);
    std::vector<double> lats(pointCount);

    // Copy the coordinates into the arrays 
    for (int i = 0; i < pointCount; ++i) 
    {
        lons[i] = geoPolygon.coordinateAt(i).longitude();
        lats[i] = geoPolygon.coordinateAt(i).latitude();
    }

    // Perform the core transformation using the PROJ library
    int result = proj_trans_generic(
        m_projTransform, PJ_FWD,
        lons.data(), sizeof(double), pointCount,
        lats.data(), sizeof(double), pointCount,
        nullptr, 0, 0, nullptr, 0, 0
    );

    if (result < 0)
    {
        qCritical() << "CGeoToLambert:  Failed to transform coordinates:" << proj_errno_string(proj_context_errno(PJ_DEFAULT_CTX));
        screenPolygon.clear();
        return -1;
    }
    return true;
}*/

bool CGeoToLambert::transformPoint(const QGeoCoordinate& geoPoint, QPointF& planarPoint)
{
    // Ensure the transformation object has been created
    if (!m_projTransform)
    {
        qWarning() << "CGeoToLambert: No projection created in transformPoint. Call CreateProjection first.";
        return false;
    }

    //Transform the geoPoint to projection
    PJ_COORD pjGeoPoint;
    pjGeoPoint.xyz.x = geoPoint.longitude();
    pjGeoPoint.xyz.y = geoPoint.latitude();
    PJ_COORD pjProjPoint = proj_trans(m_projTransform, PJ_FWD, pjGeoPoint);

    planarPoint.setX(pjProjPoint.xyz.x);
    planarPoint.setY(pjProjPoint.xyz.y);
    return true;
}