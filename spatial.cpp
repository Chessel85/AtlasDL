//spatial.cpp

#include "spatial.h"
#include <QCoreApplication>
#include <QSet>
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "way.h"
#include "relation.h"
#include "multiPolygon.h"
#include "polygon.h"
#include "GeoToLambert.h"
#include "dbmanager.h"
#include "layers.h"
#include "element.h"


//Set up logging category hierarchy
Q_LOGGING_CATEGORY(spatialManagement, "spatial.management")

CSpatial::CSpatial( CDbManager* dbmanager, QObject* parent )
    : m_DbManager(dbmanager), QObject( parent)
{
    // Check for null pointer in release builds (Q_ASSERT for debug)
    if( !m_DbManager ) 
    {
        qFatal("CSpatial initialized with null CDBManager!");
    }
}

CSpatial::~CSpatial()
{
    //Tidying up is done by Qt obbject parent hierarchy 
}

bool CSpatial::removeAlreadyProcessedRelationIDs(QList<quint64>& relationIDs)
{
    // Check for an empty list to avoid unnecessary work.
    if (relationIDs.isEmpty()) 
    {
        return true;
    }

    // Construct a whole list of question marks for populating later in the query 
    QString placeHolders;
    placeHolders.reserve(relationIDs.size() * 2); // Reserve space for performance.
    for (int i = 0; i < relationIDs.size(); ++i) 
    {
        placeHolders += "?";
        //Add a comma unless the last question mark 
        if (i < relationIDs.size() - 1) 
        {
            placeHolders += ", ";
        }
    }

    QString query = QString("SELECT relationID FROM tbl_Relations WHERE relationID IN (%1);").arg(placeHolders);

    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQuery(query );
    if (!stmt)
        return false;

    // Bind all the relation IDs to the query
    int rc = 0;
    for (int i = 0; i < relationIDs.size(); ++i) 
    {
        rc = sqlite3_bind_int64(stmt, i+1, relationIDs.at(i));
        if (rc != SQLITE_OK)
        {
            qCCritical(spatialManagement) << "Failed to bind " << relationIDs.at(i) << " to query with error " << m_DbManager->errorMessage();
            sqlite3_finalize(stmt);
            return false;
        }
    }

    // Execute the query
    QSet<quint64> existingIDs;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        quint64 value = sqlite3_column_int64(stmt, 0  );
        existingIDs.insert(value);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data from existing relation query.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    // Remove the existing IDs from the original list using iterators
    auto it = relationIDs.begin();
    while (it != relationIDs.end()) 
    {
        if (existingIDs.contains(*it)) 
        {
            it = relationIDs.erase(it);
        }
        else {
            ++it;
        }
    }

    return true;
}

bool CSpatial::selectGeometryId( const QString& geometryName, int& geometryId )
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/selectGeometryId.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    QByteArray utf8Name;
    bool ok = bindText(stmt, 1, geometryName, utf8Name);
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        geometryId= sqlite3_column_int(stmt, 0);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error retreiving geometry id from name" << geometryName;
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::selectSimplifiedWayList(int polygonId, const QString& simplificationColumn, QList<SimplifiedWayEntry>& simplifiedWayList)
{
    //Prepare for the SQLite call 
    QString wayX = "way" + simplificationColumn;
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectWayListWithSimplifiedWayWkt.sql", wayX, wayX  );
    if (!stmt)
        return false;

    // Bind parameters 
    bool ok = bindInt(stmt, 1, polygonId);
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        SimplifiedWayEntry swe;
        swe.polygonId = polygonId;
        swe.innerRingId = sqlite3_column_int(stmt, 0);
        swe.sequenceNumber = sqlite3_column_int(stmt, 1);
        swe.wayId = sqlite3_column_int64(stmt, 2);
        int wktLength = sqlite3_column_bytes(stmt, 3);
        if (wktLength > 0)
        {
            std::string wayKT = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            swe.wkt = wayKT;
        }
        simplifiedWayList.append(swe);
    }    
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error retreiving simplified wkt for polygon" << polygonId;
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::InsertLayer(const QString& layerName, int geometryId )
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertLayer.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    QByteArray utf8Name;
    bool ok = bindText(stmt, 1, layerName, utf8Name);
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, geometryId);
    if (!ok)
        return false;

    // Execute the query tolerating it might already exist 
    ok = executeStatement(stmt, true);
    if (!ok)
    {
        qCWarning(spatialManagement) << "Inserting layer" << layerName << "failed";
        return false;
    }

    return true;
}

bool CSpatial::readLayerNames( CLayers& layerNames )
{
        //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectlayerIds.sql");
    if (!stmt)
        return false;

    // Execute the query
    int rc = 0;
    while( ( rc = sqlite3_step(stmt) ) == SQLITE_ROW )
    {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0 ));
        QString qName = QString::fromStdString(name);
        int layerId = sqlite3_column_int(stmt, 1);
        layerNames.addLayer(  qName, layerId );
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data reading layer names.";
        sqlite3_finalize(stmt);
        return false;
    }

        //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

return true;
}

bool CSpatial::insertWays(CRelation* relation)
{
    //Insert ways with their first and last node id  plus the geometry itself 
    //Get the relation to construct the ways to put in 
    QString values;
    relation->constructWaysForInserting(values);

    //Call generic method for inserting way geometries 
    bool ok = insertWayGeometries(values);
    if (!ok)
    {
        return false;
    }

    return ok;
}

bool CSpatial::insertPolygonData(quint64 relationId, int layerId, QString& wayIdsAsText, QString& wayListAsText)
{
    //Start transaction
    bool ok = beginTransaction();
    if (!ok)
        return false;

    //Insert the polygon geometry from the list of ways passed in as a values string
    sqlite3_stmt* stmt = prepareQueryFromFile( "Insert/InsertPolygonsFromWays.sql", wayIdsAsText);
    if (!stmt)
    {
        rollbackTransaction();
        return false;
    }

    ok = executeStatement(stmt, false);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

        //Get the polygon id autogenerated from the previous insert 
    sqlite3_int64 rowId = sqlite3_last_insert_rowid(m_DbManager->getDbHandle());
        QString polygonId = QString::number( rowId );

        //Insert the way list 
        wayListAsText.replace("<polygonId>", polygonId);
        stmt = prepareQueryFromFile("insert/InsertPolygonWayList.sql", wayListAsText);
        if (!stmt)
        {
            rollbackTransaction();
            return false;
        }

    // Execute the query
    ok = executeStatement(stmt, true);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    //Insert the polygon owner 
    ok = insertPolygonOwner(rowId, 3, relationId);
    if (!ok )
    {
        rollbackTransaction();
        return false;
    }

    //Add the polygon to the layer polygon table
    ok = insertLayerPolygon(rowId, layerId);
    if( !ok )
    {
        rollbackTransaction();
        return false;
    }


    //Commit all of the above
    ok = commitTransaction();
    if (!ok )
    {
        rollbackTransaction();
        return false;
    }

    return true;
}

bool CSpatial::insertPolygonOwner(int polygonId, int elementType, quint64 elementId )
{
    //Insert the polygon owner 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertPolygonOwner.sql");
    if (!stmt)
        return false;

    //Bind polygon id, element type and element id 
    bool ok = bindInt64(stmt, 1, polygonId);
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, elementType); 
    if (!ok)
        return false;
    ok = bindInt64(stmt, 3, elementId );
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, true);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::updatePolygonData(int polygonId, QString& wayIdsAsText, QString& wayListAsText)
{
    //Start a transaction
    bool ok = beginTransaction();
    if( !ok )
        return false;

    //Update the polygon geometry from the list of ways passed in as a values string
    sqlite3_stmt* stmt = prepareQueryFromFile("Update/UpdatePolygonFromWays.sql", wayIdsAsText);
    if (!stmt)
    {
        rollbackTransaction();
        return false;
    }

    //Bind the parameters
    ok = bindInt(stmt, 1, polygonId);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    ok = executeStatement(stmt, false);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    //Remove existing waylist info and replace with new info 
    QString sPolygonId = QString::number(polygonId);

    stmt = prepareQueryFromFile("delete/DeletePolygonWayList.sql", wayListAsText);
    if (!stmt)
    {
        rollbackTransaction();
        return false;
    }

    //Bind parameters
    ok = bindInt(stmt, 1, polygonId);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    // Execute the query
    ok = executeStatement(stmt, true);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

        wayListAsText.replace("<polygonId>", sPolygonId);
    stmt = prepareQueryFromFile("insert/InsertPolygonWayList.sql", wayListAsText);
    if (!stmt)
    {
        rollbackTransaction();
        return false;
    }

    // Execute the query
    ok = executeStatement(stmt, true);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    //Commit the above
    ok = commitTransaction();
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    return true;
}

bool CSpatial::insertWayGeometries(const QString& values  )
{
    sqlite3_stmt* stmt = prepareQueryFromFile("Insert/InsertWay.sql", values );
    if (!stmt)
        return false;

    // Execute the query
    bool ok = executeStatement(stmt, true);
    if( !ok ) 
        return false;

return true;
}

bool CSpatial::insertPolygonGeometry(quint64 relationId, int polygonId)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertConstructedPolygonFromWayList.sql");
    if (!stmt)
        return false;

    //Bind parameters
    int elementType = 3;
    bool ok = bindInt(stmt, 1, elementType);
    if (!ok)
        return false;
    ok = bindInt64(stmt, 2, relationId);
    if (!ok)
        return false;
    ok = bindInt(stmt, 3, polygonId);
    if (!ok)
        return false;
    ok = bindInt64(stmt, 4, relationId);
    if (!ok)
        return false;
    ok = bindInt(stmt, 5, polygonId);
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::UpdatePolygonGeometryFromWkt(int polygonId, const QString& polygonColumn, const QString& wkt)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("Update/UpdatePolygonFromWkt.sql", polygonColumn);
    if (!stmt)
        return false;

    //Bind parameters
    QByteArray utf8Wkt;
    bool ok = bindText(stmt, 1, wkt, utf8Wkt);
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, polygonId);
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::updateWaysWithSimplification(const QString& column, int tolerance)
{
    //Prepare for the SQLite call 
    QString wayColumn = "way" + column;
    sqlite3_stmt* stmt = prepareQueryFromFile("Update/UpdateWaysWithSimplification.sql", wayColumn );
    if (!stmt)
        return false;

    //Bind parameters
    bool ok = bindInt(stmt, 1, tolerance);
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::updateAreas()
{
    //Update the polygon areas 
    sqlite3_stmt* stmt = prepareQueryFromFile("Update/UpdatePolygonAreas.sql" );
    if (!stmt)
        return false;

    bool ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    //Update the relation areas 
    stmt = prepareQueryFromFile("Update/UpdateRelationAreas.sql");
    if (!stmt)
        return false;

     ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::updatePolygonsWithLayersAndColours()
{
    //Update the polygons table with the layer of each polygon and the colour index to draw it 
    bool ok = m_DbManager->executeSqlFile( SCRIPTS_PATH "Update/UpdatePolygonsWithLayerAndColour.sql");
    if (!ok)
        return false;

    //And another query specifically for the country polygons
    ok = m_DbManager->executeSqlFile("scripts/Update/UpdatePolygonsWithLayerAndColourForCountries.sql");
    if (!ok)
        return false;


    return true;
}

bool CSpatial::preprocessLinksContains()
{
    //Update links table with contains relationship  
    bool ok = m_DbManager->executeSqlFile( "scripts/Insert/InsertLinksWithContains.sql" );
    if (!ok)
        return false;

    return true;
}

bool CSpatial::preprocessLinksBorders()
{
    //Update links table with borders relationship   based on shared ways and no contains relationship 
    bool ok = m_DbManager->executeSqlFile("scripts/Insert/InsertLinksWithBorders.sql");
    if (!ok)
        return false;

    //Add the length of the border
    ok = m_DbManager->executeSqlFile("scripts/Insert/InsertLinksWithBorderLengths.sql");
    if (!ok)
        return false;

    return true;
}
bool CSpatial::contains(CPolygon* polygon, CPolygon* innerRing, bool& result)
{
    //Build polygons from the ways in each ring 
    QString outer = polygon->getWayIdListAsText();
    QString inner = innerRing->getWayIdListAsText();

        //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectPolygonContainsPolygon.sql", outer, inner );
    if (!stmt)
        return false;

        int rc = 0;
        if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            int contains = sqlite3_column_int(stmt, 0);
            if (contains == 1)
                result = true;
            else
                result = false;
        }
        else
        {
            qCCritical(spatialManagement) << "Error processing retrieved data from polygon contains polygon query";
            sqlite3_finalize(stmt);
            return false;
        }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);
    return true;
}

bool CSpatial::insertTags(const CTags&  tags, int elementType, quint64 elementId )
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertTag.sql");
    if (!stmt)
        return false;

    //Loop through the list
    bool ok = false;
    for (auto const& [k, v] : tags.asKeyValueRange())
    {
        //Bind values to statement
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        ok = bindInt(stmt, 1, elementType);
        if (!ok)
            return false;
        ok = bindInt64(stmt, 2, elementId);
        if (!ok)
            return false;
        QByteArray utf8k;
        ok = bindText(stmt, 3, k, utf8k);
        if (!ok)
            return false;
        QByteArray utf8v;
        ok = bindText(stmt, 4, v, utf8v);
        if (!ok)
            return false;

        // Execute the query
        int rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            qCCritical(spatialManagement) << "Error inserting tag.";
            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);

    return true;
}




bool CSpatial::insertCountry(CRelation* relation)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertCountry.sql" );
    if (!stmt)
        return false;

    bool ok = bindInt64(stmt, 1, relation->getId() );
    if (!ok)
        return false;
    QString name = relation->getName();
    QByteArray utf8Name;
    ok  = bindText(stmt, 2, name, utf8Name );
    if (!ok )
        return false;
    CBoundingBox bb = relation->getBoundingBox();
    ok = bindDouble(stmt, 3, bb.south() );
    if ( !ok )
        return false;
    ok = bindDouble(stmt, 4, bb.west());
    if (!ok)
        return false;
    ok = bindDouble(stmt, 5, bb.north());
    if (!ok )
        return false;
    ok = bindDouble(stmt, 6, bb.east());
    if (!ok)
        return false;
    const CNode* labelNode = relation->getLabelNode();
    double labelX=0.0, labelY=0.0;
    if (labelNode)
    {
        labelX = labelNode->getLatitude().toDouble();
            labelY  = labelNode->getLongitude().toDouble();
    }
    ok = bindDouble(stmt, 7, labelX);
    if (!ok )
        return false;
    ok = bindDouble(stmt, 8, labelY);
    if (!ok )
        return false;
    double midX, midY;
    bb.getCentre(midX, midY );
    ok = bindDouble(stmt, 9, midX );
    if (!ok)
        return false;
    ok = bindDouble(stmt, 10, midY);
    if (!ok)
        return false;


            // Execute the query
    ok = executeStatement(stmt, true );
    if (!ok)
        return false;

    return true;
}

bool CSpatial::insertLayerPolygon( int polygonId, int layerId  )
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertLayerPolygon.sql");
    if (!stmt )
        return false;

    //Bind data
    bool ok  = bindInt(stmt, 1, layerId );
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, polygonId );
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, true);
    if( !ok)
        return false;

    return true;
}

bool CSpatial::identifyBorders(bool maritime)
{
    //Two different queries for maritime and non-maritime borders 
    if (maritime)
        return true;

    bool ok = m_DbManager ->executeSqlFile( "scripts/insert/InsertBorderingRelations.sql" );
    if( !ok)
        return false;

    return true;
}

bool CSpatial::insertWays(WayMap& ways, int layerId  )
{
    //Check there are ways to process
    if (ways.isEmpty() )
    {
        qCWarning(spatialManagement) << "Inserting way data but the way collection is empty.";
        return true;
    }

    //add way geometries to the spatial way table 
    //Need to construct the values to call a generic insert method 
    QString values = "";

    for (CWay* way : ways)
    {
        //Follow format of wayId, first node id, last node id, way geometry 
        QString v = "(" + QString::number(way->getId()) + ",";
        CNode* firstNode = way->getFirstNode();
        v += QString::number(firstNode->getId()) + ",";
        CNode* lastNode = way->getLastNode();
        v += QString::number(lastNode->getId()) + ",";
        way->constructNodesWkt(v);
        v += "),";
        values += v;
    }
    //Replace trailing comma with a semicolon to close off the query
    values.replace(values.size() - 1, 1, ';');

    bool ok = insertWayGeometries(values);
    if (!ok)
        return false;

        //Insert ways into the layer way table 
    ok = insertLayerWays(ways, layerId);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::insertLayerWays(WayMap& ways, int layerId)
{
    //Construct the values to insert 
    QString values = "";
    for (CWay* way : ways)
    {
        QString v = "(" + QString::number( layerId ) + "," + QString::number(way->getId()) + "),";
        values += v;
    }
    //Replace last  comma with a semicolon to close the query off
    values.replace(values.size() - 1, 1, ";");

    //State the query directly as not using parameters but constructing entire query in the string 
    QString query = "INSERT OR IGNORE INTO tbl_layerWays  (layerId, wayId ) VALUES " + values;

    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQuery(query);
    if (!stmt)
        return false;

    // Execute the query
    bool ok = executeStatement(stmt, true);

return ok;
}

bool  CSpatial::getUnsharedWaysInsideCountry(int countryId, QList<WayEntry>& wayList)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectCountryPolygonPerimeterWays.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    bool ok = bindInt(stmt, 1, countryId );
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        WayEntry we;
        we.wayId = sqlite3_column_int64(stmt, 0);
        we.firstNodeId = sqlite3_column_int64(stmt, 1);
        we.lastNodeId = sqlite3_column_int64(stmt, 2);
        we.south = sqlite3_column_double(stmt, 3);
        we.west = sqlite3_column_double(stmt, 4);
        we.north = sqlite3_column_double(stmt, 5);
        we.east = sqlite3_column_double(stmt, 6);

        //Get start and end node coordinates
        we.startPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        we.startPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        we.endPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        we.endPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        wayList.append(we);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting country perimeter ways";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool  CSpatial::getCoastlineWaysInsidePolygon( int polygonId, QList<WayEntry>& coastlineWays, int coastlineLayerId )
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectCoastlineInsidePolygon.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    bool ok = bindInt(stmt, 1, polygonId );
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, coastlineLayerId);
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        WayEntry we;
        we.wayId = sqlite3_column_int64(stmt, 0);
        we.firstNodeId = sqlite3_column_int64(stmt, 1);
        we.lastNodeId = sqlite3_column_int64(stmt, 2);
        we.south = sqlite3_column_double(stmt, 3);
        we.west = sqlite3_column_double(stmt, 4);
        we.north = sqlite3_column_double(stmt, 5);
        we.east = sqlite3_column_double(stmt, 6);

        //debug code
        if (we.wayId == 85765543  || we.wayId == 126855339 || we.wayId == 345004356 )
            int k = 0;

        //Get start and end node coordinates
        we.startPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        we.startPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        we.endPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        we.endPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        coastlineWays.append(we);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting coastline ways inside polygon.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}


bool CSpatial::getTerritorialWatersPolygons(QList<element>& territorialWatersParents, int territorialWatersLayerId)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectTerritorialWaterPolygons.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    bool ok = bindInt(stmt, 1, territorialWatersLayerId );
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        element twParent;
        twParent.polygonId= sqlite3_column_int(stmt, 0);
        twParent.elementType= sqlite3_column_int(stmt, 1);
        twParent.elementId = sqlite3_column_int64(stmt, 2);

        territorialWatersParents.append(twParent);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting territorial water polygons.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::associateIslandsToCountry(int twPolygonId, int elementType, quint64 elementId, int islandLayerId)
{
    //Query identifies island polygons inside a given territorial water polygon and adds an entry to polygon owner table to associate island to same country as territorial water 
    sqlite3_stmt* stmt = prepareQueryFromFile("insert/InsertIslandPolygonOwners.sql");
    if (!stmt)
        return false;

    //Bind parameters 
    bool ok = bindInt(stmt, 1, elementType);
    if (!ok)
        return false;
    ok = bindInt64(stmt, 2, elementId );
    if (!ok)
        return false;
    ok = bindInt(stmt, 3, twPolygonId);
    if (!ok)
        return false;
    ok = bindInt(stmt, 4, islandLayerId);
    if (!ok)
        return false;


    // Execute the query
    ok = executeStatement(stmt, true);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::CountIslandsByCountry()
{
    //Lists out each country relation and the number of islands associated with it 
    sqlite3_stmt* stmt = prepareQueryFromFile( "Select/SelectCountIslandsByCountry.sql" );
    if (!stmt)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        //quint64 relationId = sqlite3_column_int64(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        QString qName = QString::fromStdString(name);
        int numIslands = sqlite3_column_int64(stmt, 2);
        qCInfo(spatialManagement) << name << "has" << numIslands << "islands.";
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data from existing relation query.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);


    return true;
}

bool CSpatial::getWayWkt(quint64 wayId, enumDirection direction, const QString& wayColumn, QString& wkt)
{
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectWayWkt.sql", wayColumn, wayColumn );
    if (!stmt)
        return false;

    //Bind the parameters
    int reverse = direction == backward ? 1 : 0;
    bool ok = bindInt(stmt, 1, reverse);
    if (!ok)
        return false;
    ok = bindInt64(stmt, 2, wayId );
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int wktLength = sqlite3_column_bytes(stmt, 0);
        if (wktLength > 0)
        {
            std::string sWkt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            wkt = QString::fromStdString(sWkt);
        }
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing get way wkt.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

    bool CSpatial::getPolygonWayList(int polygonId, QList<PolygonWayEntry>& polygonWayList )
{
    //Retrieve the polygon way list based on given polygon id 
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectWayList.sql");
    if (!stmt)
        return false;

    // Bind parameters 
    bool ok  = bindInt(stmt, 1, polygonId);
    if (!ok )
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        PolygonWayEntry pwe;
        pwe.polygonId = polygonId;
        pwe.innerRingId = sqlite3_column_int(stmt, 0);
        pwe.sequenceNumber = sqlite3_column_int(stmt, 1);
        pwe.direction = (enumDirection)sqlite3_column_int(stmt, 2);
        pwe.wayId = sqlite3_column_int64(stmt, 3);
        //debug code
        if (pwe.wayId == 45675673)
            int k = 0;

        quint64 firstNodeId = sqlite3_column_int64(stmt, 4);
        quint64 lastNodeId = sqlite3_column_int64(stmt, 5);

        //Actual coordinates 
        std::string startPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        std::string startPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        std::string endPointX = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        std::string endPointY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));

        //Assign variables depending on direction of way 
        if (pwe.direction == forward)
        {
            pwe.headNodeId = firstNodeId;
            pwe.tailNodeId = lastNodeId;
            pwe.headNodeX = startPointX;
            pwe.headNodeY = startPointY;
            pwe.tailNodeX = endPointX;
            pwe.tailNodeY = endPointY;
        }
        else
        {
            pwe.headNodeId = lastNodeId;
            pwe.tailNodeId = firstNodeId;
            pwe.headNodeX = endPointX;
            pwe.headNodeY = endPointY;
            pwe.tailNodeX = startPointX;
            pwe.tailNodeY = startPointY;
        }

        //Bounding rectangle 
        pwe.south = sqlite3_column_double(stmt, 6);
        pwe.west = sqlite3_column_double(stmt, 7);
        pwe.north= sqlite3_column_double(stmt, 8);
        pwe.east = sqlite3_column_double(stmt, 9);

        polygonWayList.append(pwe);
    }
    if (rc != SQLITE_DONE )
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting polygon way list.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::getCountryRelations(QList<quint64>& relationIds, QStringList& countryNames, int countryLayerId)
{
    //Retrieve all the relation ids that have polygons in the country layer 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectCountryRelations.sql");
    if (!stmt)
        return false;

    //Bind parameters
    bool ok = bindInt(stmt, 1, countryLayerId );
    if (!ok)
        return false;

        // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int relationId = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        QString sname = QString::fromStdString(name);

        relationIds.append(relationId );
        countryNames.append(sname);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting country relations.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::getCountryPolygons( quint64 relationId, QList<int>& polygonIds )
{
    //Retrieve all the polygon ids in the layer polygon table for countries 
    sqlite3_stmt* stmt = prepareQueryFromFile( "select/SelectCountryPolygons.sql" );
    if (!stmt)
        return false;

    //Bind parameters
    bool ok = bindInt64(stmt, 1, relationId);
    if (!ok)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int polygonId = sqlite3_column_int(stmt, 0);
        polygonIds.append(polygonId);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting country polygons.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}


bool CSpatial::getCountryIds(QList<QPair<quint64,int>>& countryIds )
{
    //Get all the country ids in the tbl_countries table 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectCountryIds.sql");
    if (!stmt)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        quint64 countryId = sqlite3_column_int64(stmt, 0);
        int colourIndex = sqlite3_column_int(stmt, 1);
        countryIds.append(qMakePair( countryId, colourIndex) );
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing data when retrieving country ids.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::getAllPolygonIds(QList<int>& polygonIds)
{
    //Get all the polygon ids in the polygon table 
    sqlite3_stmt* stmt = prepareQueryFromFile("select/SelectAllPolygonIds.sql");
    if (!stmt)
        return false;

    // Execute the query
    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int polygonId = sqlite3_column_int(stmt, 0);
        polygonIds.append(polygonId);
    }
    if (rc != SQLITE_DONE)
    {
        qCCritical(spatialManagement) << "Error processing retrieved data selecting all polygons.";
        sqlite3_finalize(stmt);
        return false;
    }

    //Finished with sqlite3 query 
    sqlite3_finalize(stmt);

    return true;
}

bool CSpatial::deleteNullPolygons()
{
    //Delete null polygons which cascade deletes from other tables referencing them 
    //Null polygons look like they can happen due to malformed single ways which somehow look like a closed loop when they are not actually like this 
    sqlite3_stmt* stmt = prepareQueryFromFile("delete/DeleteNullPolygons.sql" );
    if (!stmt)
        return false;

    // Execute the query
    bool ok = executeStatement(stmt, false );
    if (!ok)
        return false;

    return true;
}

bool CSpatial::deletePolygonGeometry(int polygonId)
{
    //Delete a specific polygon 
    //Prepare for the SQLite call 
    sqlite3_stmt* stmt = prepareQueryFromFile("Delete/DeletePolygon.sql");
    if (!stmt)
        return false;

    //Bind parameters
    bool ok = bindInt(stmt, 1, polygonId );
    if (!ok)
        return false;

    // Execute the query
    ok = executeStatement(stmt, false);
    if (!ok)
        return false;

    return true;
}

bool CSpatial::updatePolygonVisibility(int zoomBand, double areaThreshold)
{
    //Update each polygon based on area to say at what zoom band it is included in display queries 
    sqlite3_stmt* stmt = prepareQueryFromFile("Update/UpdatePolygonVisibility.sql");
    if (!stmt)
        return false;

    //Bind parameters
    bool ok = bindDouble(stmt, 1, areaThreshold);
    if (!ok)
        return false;
    ok = bindInt(stmt, 2, zoomBand );
    if (!ok)
        return false;

        // Execute the query
    ok = executeStatement(stmt, false);
    if( !ok )
    {
        qCCritical(spatialManagement) << "Error updating visibility of polygons.";
        return false;
    }

    return true;
}

bool CSpatial::transferPolygonBetweenLayers(int polygonId, int sourceLayerId, int targetLayerId)
{
    //Want to move a polygon from one layer to another so delete from layer polygon table and then insert  tolerating constraints on the insert in case it is already there 
    //Start transaction
    bool ok = beginTransaction();
    if (!ok)
        return false;

    //Delete the polygonId first 
    sqlite3_stmt* stmt = prepareQueryFromFile("Delete/DeleteLayerPolygonId.sql" );
    if (!stmt)
    {
        rollbackTransaction();
        return false;
    }

    //Bind statements
    ok = bindInt(stmt, 1, sourceLayerId );
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }
    ok = bindInt(stmt, 2, polygonId);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    ok = executeStatement(stmt, false);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    //Insert the polygon id into the target layer 
    ok = insertLayerPolygon(polygonId, targetLayerId);
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    //Commit all of the above
    ok = commitTransaction();
    if (!ok)
    {
        rollbackTransaction();
        return false;
    }

    return true;
}


sqlite3_stmt* CSpatial::prepareQuery(const QString& query )
{
    //Check for valid database
    sqlite3* db = m_DbManager->getDbHandle();
    if (!db)
    {
        qCCritical(spatialManagement) << "Database handle is NULL. Cannot prepare statement.";
        return nullptr;
    }

    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.toUtf8().constData(), -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Prepare statement failed with error:" << m_DbManager->errorMessage();
        sqlite3_finalize(stmt);
        return nullptr;
    }
    return stmt;
}

sqlite3_stmt* CSpatial::prepareQueryFromFile(const QString& filename, QString values1, QString values2 )
{
    QString query = m_DbManager->getQueryFromScript("scripts/" + filename);
    if (query == "")
    {
        qCCritical(spatialManagement) << "Unable to load query from" << filename;
        return nullptr;
    }


    //Check if have values to replace  in the query 
    if (values1 != "")
    {
        int index1 = query.indexOf("???");
        if (index1 != -1 )
            query.replace(index1, 3, values1);
    }

    if (values2 != "")
    {
        int index2 = query.indexOf("???");
        if (index2 != -1)
            query.replace(index2, 3, values2);
    }



    return prepareQuery(query);
}

bool CSpatial::bindText(sqlite3_stmt* stmt, int index, const QString& text, QByteArray& utf8Text )
{
    utf8Text = text.toUtf8();
    int rc = sqlite3_bind_text(stmt, index, utf8Text.constData(), utf8Text.size(), SQLITE_STATIC);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Bind text failed for index" << index << "with error:" << m_DbManager->errorMessage();
        sqlite3_finalize( stmt );
        return false;
    }
    return true;
}

bool CSpatial::executeStatement(sqlite3_stmt* stmt, bool ignoreConstraint)
{
    int rc = sqlite3_step(stmt);

    bool success1 = (rc == SQLITE_DONE);
    bool success2 = (ignoreConstraint && rc == SQLITE_CONSTRAINT);
    bool success = success1 || success2;

    if (!success)
    {
        qCCritical(spatialManagement) << "Error executing SQL statement:" << m_DbManager->errorMessage();
    }

    sqlite3_finalize(stmt);
    return success;
}

bool CSpatial::bindInt64(sqlite3_stmt* stmt, int index, quint64 value)
{
    int rc = sqlite3_bind_int64(stmt, index, value);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Bind int64 failed for index" << index << "with error:" << m_DbManager->errorMessage();
        sqlite3_finalize(stmt);
        return false;
    }
    return true;
}

bool CSpatial::bindInt(sqlite3_stmt* stmt, int index, int value)
{
    int rc = sqlite3_bind_int(stmt, index, value);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Bind int failed for index" << index << "with error:" << m_DbManager->errorMessage();
        sqlite3_finalize(stmt);
        return false;
    }
    return true;
}


bool CSpatial::bindDouble(sqlite3_stmt* stmt, int index, double value)
{
    int rc = sqlite3_bind_double(stmt, index, value);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Bind double failed for index" << index << "with error:" << m_DbManager->errorMessage();
        sqlite3_finalize( stmt );
        return false;
    }
    return true;
}

bool CSpatial::beginTransaction()
{
    char* errMsg;
    int rc = sqlite3_exec(m_DbManager->getDbHandle(), "BEGIN TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Error beginning transaction:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool CSpatial::commitTransaction()
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_DbManager->getDbHandle(), "COMMIT TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Failed to commit transaction:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool CSpatial::rollbackTransaction()
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_DbManager->getDbHandle(), "COMMIT TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        qCCritical(spatialManagement) << "Failed to rollback transaction:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}


