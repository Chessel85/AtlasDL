//geodeskReader.h
#pragma once

#include "geodesk/geodesk.h"
#include "using.h"

class CNode;
class CWay;
class CRelation;
class CProcessor;
class CSpatial;

class CGeodeskReader 
{
//Constructor
public:
    explicit CGeodeskReader();
	~CGeodeskReader();

//Methods
public:
    bool readRelations( CProcessor* processor, int currentLayerId , bool writeCountries );
    bool readWays( CSpatial* spatial, int currentLayerId );
    void setGeodeskSource(const QString& source);
    void setGeodeskFilter(const QString& filter);
    void setGeodeskArea(const QString& areaFilter);
    bool addExclusion(const QString& excludedRelation);
    void clearFilters();

private:
    void getGeodeskArea(double& minX, double& minY, double& maxX, double& maxY);
    bool isExcluded(std::string& relationName );
    void deleteMasters();
    CNode* getNode(quint64 nodeId, double longitude, double latitude );
    bool readWaysInBoundingBox(double south, double west, double north, double east, CSpatial* spatial, int currentLayerId );

//Member variables
private:
    QString m_geodeskSource;
    QString m_geodeskFilter;
    QString m_geodeskAreaFilter;
    QStringList m_excludedTags;

    //Master lists
    NodeMap  m_nodesMaster;
    WayMap m_waysMaster;
    RelationMap m_relationsMaster;
};
