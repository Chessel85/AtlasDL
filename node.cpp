//Node.cpp

#include "Node.h"
#include <QDebug>
#include <cstdlib> 


CNode::CNode(quint64 id, const char* longitude, const char* latitude)
    : CBaseOSMObject(id),
    m_longitude(longitude),
    m_latitude(latitude)
{
    char* lonEndPtr;
    double lon = std::strtod(longitude, &lonEndPtr);
    bool lonOK = (*lonEndPtr == '\0');
    if (!lonOK)
    {
        qWarning() << "Failed to convert node ID " << m_id << "longitude (" << m_longitude << "to a double.";
    }

    char* latEndPtr;
    double lat = std::strtod(latitude, &latEndPtr); 
    bool latOK = (*latEndPtr == '\0');    if (!latOK)
        if( !latOK )
    {
        qWarning() << "Failed to convert node ID " << m_id << "latitude (" << m_latitude << "to a double.";
    }

    if (lonOK && latOK)
        m_geoCoordinate = QGeoCoordinate(lat, lon);
    else
        m_geoCoordinate = QGeoCoordinate(0, 0);
}

/*
CNode::CNode(quint64 id, QString longitude, QString latitude)
    : CBaseOSMObject(id),
    m_longitude(longitude ),
    m_latitude(latitude )
{
    bool lonOK;
    double  lon = m_longitude.toDouble(&lonOK );
    if (!lonOK )
    {
        qWarning() << "Failed to convert node ID " << m_id << "longitude (" << m_longitude << "to a double.";
    }

    bool latOK;
    double  lat = m_latitude.toDouble(&latOK );
    if (!latOK)
    {
        qWarning() << "Failed to convert node ID " << m_id << "latitude (" << m_latitude << "to a double.";
    }

    if (lonOK && latOK)
        m_geoCoordinate = QGeoCoordinate(lat, lon);
    else
        m_geoCoordinate = QGeoCoordinate(0, 0);
}
*/

QString CNode::getLongitude() const
{
    return m_longitude;
}

QString CNode::getLatitude() const
{
    return m_latitude;
}

QGeoCoordinate CNode::getCoordinate() const
{
    return m_geoCoordinate;
}

double CNode::distance(CNode* other)
{
    return m_geoCoordinate.distanceTo(other->m_geoCoordinate);
}