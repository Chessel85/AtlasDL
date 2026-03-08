//Processor.h
#pragma once

#include <QObject>
#include "wayEntry.h"
#include "simplifiedWayEntry.h"
#include "using.h"
#include <QVector>

class CSpatial;
class CPoly;
class CPaths;
class CMultipoly;
class CMultiPolygon;
class CPolygon;
class CRelation;
class CFeature;
struct PolygonWayEntry;

class CProcessor : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CProcessor(CSpatial* spatial, QObject* parent = nullptr );
	~CProcessor();

//Methods
public:
    bool removeTerritorialWaters( int countryLayerId, int coastlineLayerId, int territorialWaterLayerId);
    bool processRelation( CRelation* relation, int currentLayerId , bool writeCountries );
    bool associateIslandsToCountries(int islandLayerId, int territorialWaterLayerId);
    bool updateAreas();
    bool simplifyWaysAndPolygons();
    void setSimplificationTolerance(const QString& tolerance);
    void setSimplificationColumn(const QString& column);
    void setZoomBand( const QString& zoomBand);
    void setAreaThreshold(const QString& areaThreshold);
    bool updatePolygonVisibility();
    bool buildCountryPolygonsFromHigherAdminLevels(int layerId);


private:
    bool enforceAnticlockwisePolygons(CMultiPolygon& polygons);
    bool consolidateInnerRings(CMultiPolygon& polygons, CMultiPolygon& innerRings);
    bool insertPolygonData(quint64 relationId, int layerId, CMultiPolygon& polygons);
    bool insertPolyData(quint64 relationId, int layerId, CMultipoly& polygons);
    bool updatePolygonData(int polygonId, CPolygon* polygon );
    QString constructWayListValues( CPolygon* polygon );
    bool insertPolygonGeometries(quint64 relationId, CMultiPolygon& polygons);
    bool removeTerritorialWater(int countryLayerId, int coastlineLayerId, int territorialWaterLayerId, quint64 relationId, int polygonId);
    bool getUnsharedCountryWayList(NodeMap& nodesMaster, WayMap& waysMaster, CRelation* countryRelation, int countryId, CFeature& countryFeature);
    bool getCoastline(NodeMap& nodesMaster, WayMap& waysMaster, CRelation* coastlineRelation, int polygonId, CFeature& coastlineFeatures, int coastlineLayerId );
    bool buildPolygon(int polygonId, CPoly& countryPolygon, NodeMap& nodesMaster, WayMap& waysMaster  );
    bool buildWays(QList<PolygonWayEntry>& polygonWayList, NodeMap& nodesMaster, WayMap& waysMaster);
    void DestroyMasterMaps(NodeMap& nodesMaster, WayMap& waysMaster, RelationMap& relationsMaster );
    bool reconstructPolygon(int polygonId );
    void getAnchors(CPoly& countryPolygon, CPaths& coastalLines, QVector<quint64>& anchors );
    void constructLineStrings(CPoly& countryPolygon, CPaths& coastalLines, QVector<quint64>& anchors, CPaths& waterPaths, CPaths& landPaths);
    bool buildSimplifiedPolygonWkt(QList<SimplifiedWayEntry>& simplifiedWayList, QString& wkt);

    //Member variables
private:
    int m_simplificationTolerance;
    QString m_simplificationColumn;
    int m_zoomBand;
    double m_areaThreshold;
    CSpatial* m_spatial;
};