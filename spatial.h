//spatial.h
#pragma once

#include <QObject>
#include <QString>
#include <QPair>
#include <QList>
#include "Tags.h"
#include <sqlite3.h>
#include "wayEntry.h"
#include "PolygonWayEntry.h"
#include "simplifiedWayEntry.h"
#include "using.h"


class CDbManager;
class CRelation;
class CFeature;
class CMultiPolygon;
class CPolygon;
class CLayers;
struct element;

class CSpatial : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CSpatial(CDbManager* dbmanager = nullptr, QObject* parent = nullptr);
	~CSpatial();

//Methods
public:
    QString getDatabaseName() const;
    bool removeAlreadyProcessedRelationIDs(QList<quint64>& relationIDs);
    bool selectGeometryId(const QString& layerGeometry, int& geometryId); 
    bool selectSimplifiedWayList(int polygonId, const QString& simplificationColumn, QList<SimplifiedWayEntry>& simplifiedWayList);
    bool InsertLayer(const QString& layerName, int geometryId );
    bool identifyBordersInDifferentLayers(int layerId1, int layerId2);
    bool readLayerNames(CLayers& layers);
    bool insertWays(CRelation* relation );
    bool insertPolygonData(quint64 relationId, int layerId, QString& wayIdsAsText, QString& wayListAsText );
    bool insertCountry(CRelation* relation);
    bool updatePolygonData(int polygonId, QString& wayIdsAsText, QString& wayListAsText);
    bool updateAreas();
    bool updatePolygonsWithLayersAndColours();
    bool preprocessLinksContains();
    bool preprocessLinksBorders();
    bool insertWays( WayMap& ways, int layerId );
    bool identifyBordersInSameLayers();
    bool getLayerId(const QString& groupName, const QString& layerName, int& layerId );
    bool  getUnsharedWaysInsideCountry(int countryId, QList<WayEntry>& wayList);
    bool  getCoastlineWaysInsidePolygon(int polygonId, QList<WayEntry>& coastlineWays, int coastlineLayerId);
    bool getPolygonWayList(int polygonId, QList<PolygonWayEntry>& polygonWayList );
    bool getCountryRelations(QList<quint64>& relationIds, QStringList& countryNames, int countryLayerId   );
    bool getCountryPolygons(quint64 relationId, QList<int>& polygonIds);
    bool getCountryIds(QList<QPair<quint64, int>>& countryIds);
    bool getAllPolygonIds(QList<int>& polygonIds);
    bool contains(CPolygon* polygon, CPolygon* innerRing, bool& result);
    bool insertPolygonGeometry(quint64 relationId, int polygonId);
    bool UpdatePolygonGeometryFromWkt(int polygonId, const QString& polygonColumn, const QString& wkt );
    bool updateWaysWithSimplification(const QString& column, int tolerance );
    bool deleteNullPolygons();
    bool deletePolygonGeometry(int polygonId);
    bool updatePolygonVisibility(int stepIndex, double areaThreshold);
    bool transferPolygonBetweenLayers(int polygonId, int sourceLayerId, int targetLayerId);
    bool insertTags(const CTags& tags, int elementType, quint64 elementID);
    bool insertPolygonOwner(int polygonId, int elementType, quint64 elementId);
    bool getTerritorialWatersPolygons(QList<element>& territorialWatersParents, int territorialWatersLayerId);
    bool associateIslandsToCountry(int twPolygonId, int elementType, quint64 elementId, int islandLayerId);
    bool CountIslandsByCountry();
    bool getWayWkt(quint64 wayId, enumDirection direction, const QString& wayColumn, QString& wkt);

private:
    bool createApplicationTables(const QString& scriptsPath);
    bool insertWayGeometries(const QString& values  );
    bool insertLayerPolygon( int polygonId, int layerId );
    bool insertLayerWays(WayMap& ways, int layerId);

//Helper methods
private:
    sqlite3_stmt* prepareQuery(const QString& query);
    sqlite3_stmt* prepareQueryFromFile(const QString& filename, QString values1 = "", QString values2 = "" );
    bool bindText(sqlite3_stmt* stmt, int index, const QString& text, QByteArray& textUTF8 );
    bool executeStatement(sqlite3_stmt* stmt, bool ignoreConstraint );
    bool bindInt64(sqlite3_stmt* stmt, int index, quint64 value);
    bool bindInt(sqlite3_stmt* stmt, int index, int value);
    bool bindDouble(sqlite3_stmt* stmt, int index, double value);
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

//Member variables
private:
    CDbManager* m_DbManager;
};