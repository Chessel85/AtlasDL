//Node.h
#pragma once

#include "baseOSMObject.h"
#include <QGeoCoordinate>

class CNode : public CBaseOSMObject
{
//Constructor
public:
    //CNode(quint64 id, QString lon, QString lat);
    CNode(quint64 id, const char*lon, const char* lat);

    QString getLatitude() const;
    QString getLongitude() const;
    QGeoCoordinate getCoordinate() const;
    double distance(CNode* other);

private:
    QString m_latitude;
    QString m_longitude;
    QGeoCoordinate m_geoCoordinate;
};
