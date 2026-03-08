//Processor.cpp

#include "Processor.h"
#include <QCoreApplication>
#include <QDebug>
#include "loggingCategories.h"
#include "paths.h"
#include "poly.h"
#include "multipoly.h"
#include "multiPolygon.h"
#include "polygon.h"
#include "feature.h"
#include "relation.h"
#include "way.h"
#include "node.h" //can remove after memory leak detection done
#include "spatial.h"
#include "wayEntry.h"
#include "element.h"
#include <vector>



//Set up logging category hierarchy
Q_LOGGING_CATEGORY(ProcessorManagement, "Processor.management")

CProcessor::CProcessor(CSpatial* spatial, QObject* parent)
    : m_spatial(spatial), QObject( parent)
{
}

CProcessor::~CProcessor()
{
}

bool CProcessor::consolidateInnerRings(CMultiPolygon& polygons, CMultiPolygon& innerRings)
{
    //Iterate through the inner rings and check if bounding box lies within any polygon 
    //If it does check if polygon contains the inner ring
    //If it does then move the inner ring to the polygon
    CPolygon* innerRing = nullptr;
    innerRings.moveToFirstPolygon();
    while ( (innerRing = innerRings.getNextPolygon() ) )
    {
        //Iterate through main polygons
        CPolygon* polygon = nullptr;
        polygons.moveToFirstPolygon();
        while (polygon = polygons.getNextPolygon())
        {
            //Do a bounding box comparison 
            if (!polygon->containsMBR(innerRing))
                continue;
            //Get spatialite to do the heavy lifting of detailing if inner ring is completely contained within polygon
            bool isContained = false;
            bool ok = m_spatial->contains(polygon, innerRing, isContained);
            if (!ok)
            {
                return false;
            }
            if (isContained)
            {
                //Move the inner ring to the polygon
                polygon->addInnerRing(innerRing);
                innerRings.removeInnerRing(innerRing);
                break;
            }
        }
    }

    //There should be no inner rings left
    //Q_ASSERT(innerRings.isEmpty());
    if (!innerRings.isEmpty())
    {
        qCWarning(ProcessorManagement) << "There are " << innerRings.numPolygons() << "remaing after consolidation.";
    }

    return true;
}

bool CProcessor::removeTerritorialWaters(int countryLayerId, int coastlineLayerId, int territorialWaterLayerId)
{
    //Must have layer id values greater than 0 to be valid
    if (countryLayerId <= 0 || coastlineLayerId <= 0 || territorialWaterLayerId <= 0)
    {
        qCCritical(ProcessorManagement) << "Invalid layer id values passed to remove territorial water method.";
        return false;
    }

    //Iterate through each country  relation and each polygon within that  country 
    // Get all coastline ways within polygon  and stitch together to get coastlines of mainland 
    //For mainland coastline identify common nodes between coastline ways and country polygons 
    //For a country polygon theory is there are at most just two ways with one being the start of the intersection and the other the end of the intersection
    //Redefine this polygon by removing ways that are in the water and replacing them with the string of coastline ways 
    //Islands are loaded as a separate layer rather than matching with islands detected in this algorithm 

    //Get the list of relations that have polygons in the country layer 
    QList<quint64> relationIds;
    QStringList countryNames;
    bool ok = m_spatial->getCountryRelations(relationIds , countryNames, countryLayerId);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve relation id's for removing territorial waters from.";
        return false;
    }

    int counter = 0;
    for (int relationId : relationIds)
    {
        QString countryName = countryNames.at(counter++);
        //Only do Finland for now
        if (countryName != "Finland" )
            continue;
        QList<int> polygonIds;
        ok = m_spatial->getCountryPolygons(relationId, polygonIds);
        if (!ok)
            return false;

        qCInfo(ProcessorManagement) << countryName << "has" << polygonIds.size() << "polygons.";

        //Work through each polygon 
        int polyCounter = 1;
        for (int polygonId : polygonIds)
        {
            qCInfo(ProcessorManagement) << "Processing polygon" << polyCounter++;
            ok = removeTerritorialWater(countryLayerId, coastlineLayerId, territorialWaterLayerId, relationId, polygonId);
            if (!ok)
                return false;
        }
    }

    //Need to remove any null polygons and update areas 
    ok = m_spatial->deleteNullPolygons();
    if (!ok)
        return false;

    ok = updateAreas();
    if (!ok)
        return false;

    return true;
}

bool CProcessor::removeTerritorialWater(int countryLayerId, int coastlineLayerId, int territorialWaterLayerId, quint64 relationId, int polygonId)
{
    //Setup objects 
    NodeMap  nodesMaster;
    WayMap waysMaster;
    RelationMap relationsMaster;

    //Set up a relation to store coastline ways which feed into a coastline feature 
    CRelation* coastlineRelation = new CRelation(0);
    relationsMaster.insert(coastlineRelation->getId(), coastlineRelation);
    CFeature coastlineFeatures(coastlineRelation);

    bool ok = getCoastline(nodesMaster, waysMaster, coastlineRelation, polygonId, coastlineFeatures, coastlineLayerId);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve any coastline data for removing territorial waters Relation id" << relationId;
        DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
        return false;
    }

    //A landlocked country has no coastlines and no islands  or might be an island with no territorial water 
    CMultiPolygon& coastalLines = coastlineFeatures.getLinestrings();
    if (coastlineFeatures.isEmpty())
    {
        qCInfo(ProcessorManagement) << "No coastline or islands for this country polygon (probably landlocked or orphaned island) meaning no territorial waters.";
        DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
        return true;
    }

    //Convert the linestrings to  CPath objects 
    //One day will convert code to create these directly rather than going through a relation and a feature pair of objects 
    CPaths coastlinePaths;
    coastlinePaths.buildFromMultiPolygon(coastalLines);

    //Construct the country poly from the database table 
    CPoly countryPoly;
    ok = buildPolygon(polygonId, countryPoly, nodesMaster, waysMaster);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to get polygon way list.";
        return false;
    }

    //Find anchor points which are points on perimeter and inner rings where coastline touches 
    //May get none if  no coastlines 
    QVector<quint64> anchors;
    getAnchors(countryPoly, coastlinePaths, anchors);

    //Preparation now complete as have the anchor points where coastlines intersect with any points in country
    //Two scenarios follow.  One where there are coastlines (at least one line string bisecting the country polygon)
    //And the other where no line strings and only islands
    if (coastalLines.size() > 0 )
    {
        //identify linestrings  that are either land or sea
        qCInfo(ProcessorManagement) << "Constructing sea, land and coastal linestrings.";
        CPaths waterLinestrings, landLinestrings;
        constructLineStrings(countryPoly, coastlinePaths, anchors, waterLinestrings, landLinestrings);

        //Have coastline, water and land linestrings so stitch them together into water and land multipolygons
        CMultipoly waterPolygons;
        CMultipoly landPolygons;

        //Assemble them knowing that water polygons need the coastlines reversed to maintain perimeter anticlockwise rule  so do land first otherwise coastlines incorrectly reversed 
        landPolygons.assembleFromPaths(landLinestrings, coastlinePaths);
        waterPolygons.assembleFromPaths(waterLinestrings, coastlinePaths, true);

        qCInfo(ProcessorManagement) << "Created" << landPolygons.size() << "land polygons and" << waterPolygons.size() << "water polygons.";

        //Update the database with new polygons remembering to remove superseded polygons as well 
        ok = insertPolyData(relationId, countryLayerId, landPolygons);
        if (!ok)
        {
            qCWarning(ProcessorManagement) << "Failed to update a country polygon after removing territorial waters for relation" << relationId << "and polygon id" << polygonId;
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            return true; //Want to continue processing
        }

        ok = m_spatial->deletePolygonGeometry(polygonId);
        if (!ok)
        {
            qCWarning(ProcessorManagement) << "Failed to delete original country polygon after removing territorial waters for relation" << relationId << "and polygon id" << polygonId;
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            return true; //Want to continue processing
        }
    } //there were coastlines bisecting the country polygon 
    else if (coastlineFeatures.getPolygons().size() > 0)
    {
        qCInfo(ProcessorManagement) << "No coastal line strings so moving territorial water polygon to territorial water layer .";
        ok = m_spatial->transferPolygonBetweenLayers(polygonId, countryLayerId, territorialWaterLayerId);
        if (!ok)
        {
            qCWarning(ProcessorManagement) << "Error transferring polygon from country layer to territorial water layer.";
            return true;
        }
    }

        //Once finished processing destroy the node and way masters
        DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);


        return true;
}

bool CProcessor::buildCountryPolygonsFromHigherAdminLevels ( int layerId )
{
    //Setup objects 
    NodeMap  nodesMaster;
    WayMap waysMaster;
    RelationMap relationsMaster;

    QList<QPair<quint64, int>> countryIds;
    bool ok = m_spatial->getCountryIds(countryIds);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to get country ids for building country polygons from higher admin levels.";
        DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
        return false;
    }

    //Get all the country ids and work through each one 
    int successCount = 0;
    for (QPair<int, int> countryId : countryIds)
    {
        //Get country id  as second is the colour index 
        int cId = countryId.first;

        //Set up a relation to store country ways which feed into a country feature 
        CRelation* countryRelation = new CRelation(cId );
        relationsMaster.insert(countryRelation->getId(), countryRelation);
        CFeature countryFeature(countryRelation);

        ok = getUnsharedCountryWayList(nodesMaster, waysMaster, countryRelation, cId, countryFeature);
        if (!ok)
        {
            qCCritical(ProcessorManagement) << "Failed to get ways to deduce polygons for countries from unshared ways in higher admin levels.";
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            return false;
        }

        //Is possible that no details of higher admin levels was available for the given country so continue if this is the case
        if (waysMaster.isEmpty())
        {
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            continue;
        }

        //Move any inner rings to their containing polygons  and enforce anticlockwise rotation
        ok = consolidateInnerRings(countryFeature.getPolygons(), countryFeature.getInnerRings());
        if (!ok)
        {
            qCWarning(ProcessorManagement) << "Error consolidating inner rings to parent polygons for relation" << countryRelation->getName();
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            return false;
        }

        //Enforce anticlockwise rotation  to help with later processing
        countryFeature.getPolygons().enforceAnticlockwiseRotation();

        //Insert polygon geometry, the way list, polygon layer  and the polygon owner 
        ok = insertPolygonData(countryRelation->getId(), layerId, countryFeature.getPolygons());
        if (!ok)
        {
            qCCritical(ProcessorManagement) << "Failed to insert polygon way lists when deriving country polygon from higher admin levels for " << countryRelation->getName();
            DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
            return false;
        }

        successCount++;

        //Tidy up
        DestroyMasterMaps(nodesMaster, waysMaster, relationsMaster);
    } //country id loop 

    qCInfo(ProcessorManagement) << "Created polygons for" << successCount << "countries out of" << countryIds.size();
    updateAreas();

    return true;
}

bool CProcessor::buildPolygon(int polygonId, CPoly& countryPolygon, NodeMap& nodesMaster, WayMap& waysMaster)
{
PolygonWayList polygonWayList;
    bool ok = m_spatial->getPolygonWayList(polygonId, polygonWayList);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to get ways to build country polygon.";
        return false;
    }

    //Build the ways from the polygon way list
    buildWays(polygonWayList, nodesMaster, waysMaster);

    //Create new polygon   based on retrieved data 
    if (!polygonWayList.isEmpty())
    {
        ok = countryPolygon.buildFromWayList(polygonWayList, waysMaster);
    }

    return ok;
}

bool CProcessor::buildWays(QList<PolygonWayEntry>& polygonWayList, NodeMap& nodesMaster, WayMap& waysMaster)
{
    //Iterate through the way list to construct way objects needed later when removing territorial waters
    for (PolygonWayEntry pwe : polygonWayList)
    {
        quint64 wayId = pwe.wayId;

        //Create if it does not exist already
        if (!waysMaster.contains(wayId))
        {
            CWay* way = new CWay(wayId);
            waysMaster.insert(wayId, way);
        }
        CWay* way = waysMaster.value(wayId);

        //Create head node if needed
        if (!nodesMaster.contains(pwe.headNodeId ))
        {
            CNode* headNode = new CNode(pwe.headNodeId, pwe.headNodeX.c_str(), pwe.headNodeY.c_str() );
            nodesMaster[ pwe.headNodeId ] =  headNode;
        }
        CNode* headNode = nodesMaster[ pwe.headNodeId ];
        headNode->addParent(way->getId() );
        if (headNode->getParents().size() > 2)
        {
            qCWarning(ProcessorManagement) << "Possible segment crossing at head node" << headNode->getLongitude() << "," << headNode->getLatitude() << "for way id" << way->getId();
        }
        way->addNode(headNode);

        //Create tail node if needed
        if (!nodesMaster.contains(pwe.tailNodeId))
        {
            CNode* tailNode = new CNode(pwe.tailNodeId, pwe.tailNodeX.c_str(), pwe.tailNodeY.c_str());
            nodesMaster[pwe.tailNodeId] = tailNode;
        }
        CNode* tailNode = nodesMaster[pwe.tailNodeId];
        tailNode->addParent(way->getId() );
        if (tailNode->getParents().size() > 2)
        {
            qCWarning(ProcessorManagement) << "Possible segment crossing at tail node" << tailNode->getLongitude() << "," << tailNode->getLatitude() << "for way id" << way->getId();
        }
        way->addNode(tailNode);

                //Deal with bounding box
        CBoundingBox bb;
        bb.setBoundingBox(pwe.south, pwe.west, pwe.north, pwe.east);
        way->getBoundingBox().setBoundingBox(bb.south(), bb.west(), bb.north(), bb.east());
    }

    return true;
}

bool CProcessor::getUnsharedCountryWayList(NodeMap& nodesMaster, WayMap& waysMaster, CRelation* countryRelation, int countryId, CFeature& countryFeature )
{
    //get the ways using admin level 5 and 6 data for a country 
    QList<WayEntry> wayEntries;
    bool ok = m_spatial->getUnsharedWaysInsideCountry(countryId, wayEntries );
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve unshared ways within country id" << countryId;
        return false;
    }

    //Is possible to get no way entries if there is no higher admin level data so return true in this event
    if (wayEntries.isEmpty())
        return true;

    //Populate the relation for the country from the way entries ready for the feature object to deduce line strings and polygons 
    countryRelation->build(nodesMaster, waysMaster, wayEntries);

    //Create a feature and deduce polygons 
    ok = countryFeature.deducePolygons();
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Unable to generate polygons and line strings from country unshared ways.";
        return false;
    }

    return true;
}

bool CProcessor::getCoastline(NodeMap& nodesMaster, WayMap& waysMaster, CRelation* coastlineRelation, int polygonId, CFeature& coastlineFeatures, int coastlineLayerId )
{
    //Get coastline data in the polygon  from database
    QList<WayEntry> wayEntries;
    bool ok = m_spatial->getCoastlineWaysInsidePolygon(polygonId, wayEntries, coastlineLayerId);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve coastline data for relation" << coastlineRelation->getId();
        return false;
    }

    //Populate the relation for the coastline data from the way entries ready for the feature object to deduce line strings and polygons 
    coastlineRelation->build(nodesMaster, waysMaster, wayEntries);

        //Create a feature and deduce polygons 
    ok = coastlineFeatures.deducePolygons();
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Unable to generate polygons and line strings from coastline data.";
        return false;
    }

    return true;
}

bool CProcessor::enforceAnticlockwisePolygons(CMultiPolygon& polygons)
{
    //Iterate through polygons 
    polygons.moveToFirstPolygon();
    CPolygon* polygon = nullptr;
    while ((polygon = polygons.getNextPolygon()))
    {
        //Tell the polygon to enforce direction 
        polygon->enforceAnticlockwise();
    }

    return true;
}

bool CProcessor::processRelation( CRelation* relation, int currentLayerId , bool writeCountries )
{
    //Can immediately store the ways from the relation  meaning the way geometry and first and last nodes 
    bool ok = m_spatial->insertWays(relation );
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to insert way geometries from relation " << relation->getName();
        return false;
    }

    // Deduce polygons
    CFeature feature(relation);
    ok = feature.deducePolygons();
    if (!ok)
    {
        qCWarning(ProcessorManagement) << "Error deducing polygons for relation" << relation->getName();
        return false;
    }

    //Move any inner rings to their containing polygons  and enforce anticlockwise rotation
    ok = consolidateInnerRings(feature.getPolygons(), feature.getInnerRings());
    if (!ok)
    {
        qCWarning(ProcessorManagement) << "Error consolidating inner rings to parent polygons for relation" << relation->getName();
        return false;
    }

    //Enforce anticlockwise rotation  to help with later processing
    feature.getPolygons().enforceAnticlockwiseRotation();

    //Insert polygon geometry, the way list, polygon layer  and the polygon owner 
    ok = insertPolygonData(relation->getId(), currentLayerId, feature.getPolygons());
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to insert polygon way lists from relation " << relation->getName();
        return false;
    }

    //Write tags 
    CTags tags = relation->getTags();
    ok = m_spatial->insertTags(tags, 3, relation->getId() );
    if (!ok)
    {
        qCWarning(ProcessorManagement) << "Failed to insert relation tags for relation " << relation->getName();
        return false;
    }

    //Write country relation data if flagged
    if (writeCountries)
    {
        ok = m_spatial->insertCountry(relation);
        if (!ok)
        {
            qCCritical(ProcessorManagement) << "Failed to write country relation to country table for " << relation->getName();
            return false;
        }
    }

    return true;
}

bool CProcessor::insertPolygonData(quint64 relationId, int layerId, CMultiPolygon& polygons)
{
    //Iterate through the polygons to insert each one 
    CPolygon* polygon = nullptr;
    polygons.moveToFirstPolygon();
    while ((polygon = polygons.getNextPolygon()))
    {
        //Under one transaction insert a polygon geometry, the way list, the polygon layer  and assign the polygon owner 
        //Need to know the list of ways to construct the polygon geometry 
        QString listOfWayIds = polygon->getWayIdListAsText();

        //And need way list values
        QString wayListText = constructWayListValues(polygon);

        bool ok = m_spatial->insertPolygonData(relationId, layerId, listOfWayIds, wayListText);
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

bool CProcessor::insertPolyData(quint64 relationId, int layerId, CMultipoly& polygons)
{
    //Iterate through the polygons to insert each one 
    for( CPoly* polygon : polygons )
    {
        //Under one transaction insert a polygon geometry, the way list, the polygon layer  and assign the polygon owner 
        //Need to know the list of ways to construct the polygon geometry 
        QString listOfWayIds = polygon->getWayIdListAsText();
        listOfWayIds = QString( "(%1)").arg(listOfWayIds);

        //Need way list details 
        QString wayListText = polygon->getWayListAsText();

        bool ok = m_spatial->insertPolygonData(relationId, layerId, listOfWayIds, wayListText);
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

QString CProcessor::constructWayListValues( CPolygon* polygon )
{
    QString values = "";
    int sequenceNumber = 0;
    CDirectionalWay* dw = nullptr;
    int innerRing = 0;
    polygon->moveToFirstDirectionalWay();
    while ((dw = polygon->getNextDirectionalWay()))
    {
        sequenceNumber++;
        quint64 wayId = dw->getWayId();
        enumDirection direction = dw->getDirection();

        //Construct the actual values
        QString value = "(" +
            QString( "<polygonId>,") +
            QString::number( innerRing) + "," + 
            QString::number(sequenceNumber) + "," +
            QString::number(direction) + "," +
            QString::number(wayId) + ")";

        values += value + ",\n";
    }

        //Iterate through any inner polygons
        int numInnerrings = polygon->getNumInnerRings();
        for (int i = 0; i < numInnerrings; i++)
        {
            CPolygon* innerRing = polygon->getInnerRing(i);
            int sequenceNumber = 0;
            CDirectionalWay* dw = nullptr;
            innerRing->moveToFirstDirectionalWay();
            while ((dw = innerRing->getNextDirectionalWay()))
            {
                sequenceNumber++;
                quint64 wayId = dw->getWayId();
                enumDirection direction = dw->getDirection();

                //Construct the actual values
                QString value = "(" +
                    QString( "<polygonId>," ) +
                    QString::number(i + 1) + "," +
                    QString::number(sequenceNumber) + "," +
                    QString::number(direction) + "," +
                    QString::number(wayId) + ")";

                values += value + ",\n";
            }
        }

    //Replace trailing comma with a space 
    values.replace(values.size() - 2, 2, ' ');

    return values;
}

bool CProcessor::insertPolygonGeometries(quint64 relationId, CMultiPolygon& polygons )
{
    //For each polygon commit it to a geometry table 
    bool ok = true;
    int polygonId = 0;
CPolygon* polygon = nullptr;
    polygons.moveToFirstPolygon();
    while ((polygon = polygons.getNextPolygon()) && ok )
    {
        polygonId++;
        ok = m_spatial->insertPolygonGeometry(relationId, polygonId);
    }

    return ok;
}

void CProcessor::DestroyMasterMaps(NodeMap& nodesMaster, WayMap& waysMaster, RelationMap& relationsMaster )
{
    for (auto const& [key, node] : nodesMaster)
    {
        delete node;
    }
    nodesMaster.clear();

        for (CWay* way : waysMaster)
    {
        delete way;
    }
    waysMaster.clear();

    for (CRelation* relation : relationsMaster)
    {
        delete relation;
    }
    relationsMaster.clear();
}

bool CProcessor::updatePolygonData(int polygonId, CPolygon* polygon)
{
    //Under one transaction update the polygon geometry, remove the current way list entries and replace with new ones 
    //Need to know the list of ways to construct the polygon geometry 
    QString listOfWayIds = polygon->getWayIdListAsText();

    //And need way list values
    QString wayListText = constructWayListValues(polygon);

    bool ok = m_spatial->updatePolygonData(polygonId, listOfWayIds, wayListText );
    if (!ok)
    {
        return false;
    }

return true;
}


bool CProcessor::associateIslandsToCountries(int islandLayerId, int territorialWatersLayerId)
{
    //Validate layer ids
    if (islandLayerId <= 0 || territorialWatersLayerId <= 0)
    {
        qCCritical(ProcessorManagement) << "Invalid island or territorial waters layer ids when associating islands.";
        return false;
    }

    //Iterate through each territorial water polygon and collect any island polygons they contain and associate to owning country
    QList<element> territorialWatersParents;
    bool ok = m_spatial->getTerritorialWatersPolygons(territorialWatersParents, territorialWatersLayerId);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve territorial water polygons.";
        return false;
    }

    //Iterate through each territorial water polygon to associate with parent country 
    for (element twParent : territorialWatersParents)
    {
        int twPolygonId = twParent.polygonId;
        ok = m_spatial->associateIslandsToCountry(twPolygonId, twParent.elementType, twParent.elementId, islandLayerId);
        if (!ok)
        {
            qCCritical(ProcessorManagement) << "Failed to associate islands in territorial water polygon" << twParent.polygonId << "with parent country relation" << twParent.elementId;
            return false;
        }
    }

    //Update polygon areas
    ok = updateAreas();
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to update areas of associated islands.";
        return false;
    }

    return true;
}


bool CProcessor::updateAreas()
{
    return m_spatial->updateAreas();
}

bool CProcessor::reconstructPolygon(int polygonId)
{
    //Need to generate the simplified polygons following simplifying the ways 
    // Populate a polygon way list from the database including the simplified wkt 

    //Get construction info and wkt from the database 
    QList<SimplifiedWayEntry> simplifiedWayList;
    bool ok = m_spatial-> selectSimplifiedWayList(polygonId, m_simplificationColumn, simplifiedWayList);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to retrieve simplified way list for polygon" << polygonId;
        return false;
    }

    //Construct the full wkt 
    QString wkt;
    ok = buildSimplifiedPolygonWkt(simplifiedWayList, wkt);

    //Update the database 
    QString polygonColumn = "polygon" + m_simplificationColumn;
    ok = m_spatial->UpdatePolygonGeometryFromWkt(polygonId, polygonColumn, wkt);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to update polygon geometry from wkt for polygon" << polygonId;
        return false;
    }

return true;
}

void CProcessor::setSimplificationTolerance(const QString& tolerance)
{
    m_simplificationTolerance = tolerance.toInt();
}

void CProcessor::setSimplificationColumn(const QString& column)
{
    m_simplificationColumn = column;
}

bool CProcessor::simplifyWaysAndPolygons()
{
    //Tell spatial to simplify the way geometries in a given column in the spt_ways table to a given tolerance
    bool ok =   m_spatial->updateWaysWithSimplification(m_simplificationColumn, m_simplificationTolerance);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to update way table column" << m_simplificationColumn << "with simplified ways using tolerance" << m_simplificationTolerance;
        return true;
    }

    //Based on the simplified ways update the polygons 
    //Simplification of ways can introduce overlaps in resulting polygons so need to apply the buffer 0 trick to get rid of them
    //This is only available when using PolyFromText so need to construct the wkt for each polygon in the table
    QList<int> polygonIds;
    ok = m_spatial->getAllPolygonIds(polygonIds);
    if (!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to get all polygon ids for simplification.";
        return false;
    }

    //Iterate through the list
    for (int polygonId : polygonIds)
    {
        ok = reconstructPolygon(polygonId);
        if (!ok)
            return false;
    }

    return true;
}

void CProcessor::setZoomBand(const QString& zoomBand)
{
    m_zoomBand= zoomBand.toInt() ;
}

void CProcessor::setAreaThreshold( const QString& areaThreshold)
{
    m_areaThreshold = areaThreshold.toDouble();
}

bool CProcessor::updatePolygonVisibility()
{
    //Check variables are valid 
    if (m_zoomBand< 0 || m_zoomBand> 20 || m_areaThreshold < 0.0 )
    {
        qCCritical(ProcessorManagement) << "Invalid variables for polygon visibility with step size" << m_zoomBand<< " and area threshold" << m_areaThreshold;
        return false;
    }

    //Get the database to update the polygon table with the index value above which to show the polygon so that small areas are not returned for rendering if too small for the current step size 
    bool ok = m_spatial->updatePolygonVisibility(m_zoomBand, m_areaThreshold);
     if(!ok)
    {
        qCCritical(ProcessorManagement) << "Failed to update polygon visibility with step size" << m_zoomBand<< " and area threshold" << m_areaThreshold;
        return false;
    }

    return true;
}

void CProcessor::getAnchors(CPoly& countryPolygon, CPaths& coastalLines, QVector<quint64>& anchors )
{
    //Loop through each coastline and get any intersections with the country polygon
    QList<CPath*> paths = coastalLines.getPaths();
    for (CPath* path : paths)
    {
        countryPolygon.getIntersections(path, anchors);
    }
}

void CProcessor::constructLineStrings(CPoly& countryPolygon, CPaths& coastalLines, QVector<quint64>& anchors, CPaths& waterLinestrings, CPaths& landLinestrings)
{
    //Loop through each anchor point to find directional way with this as the head node  as all linestrings start like this 
    for (quint64 anchorHeadNodeId : anchors)
    {
        //Find this anchor head node in country polygon 
        int ring = 0;
        CDirectionalWay* headDw = nullptr;
        bool ok = countryPolygon.getDirectionalWayWithHeadNodeId(anchorHeadNodeId, headDw, ring);
        
        //And find it in the coastline and is important to know if is head or tail
        int intersectionType = coastalLines.getIntersectionType(anchorHeadNodeId);
        Q_ASSERT(intersectionType!= 0);

        //If type 1 then grow a new path recreating the territorial water section of the country polygon 
        if (intersectionType == 1)
        {
            //Construct a water way list by continuing around the country way list until get to another anchor point 
            CPath* waterPath = nullptr;
            waterPath = countryPolygon.duplicateSection(headDw->getHeadNodeId(), ring, anchors);
            waterLinestrings.addPath(waterPath);
        }
        else //type is -1 meaning a land border 
        {
            //Construct a land path by continuing around the land polygon 
            CPath* landPath = nullptr;
            landPath = countryPolygon.duplicateSection(headDw->getHeadNodeId(), ring, anchors);
            landLinestrings.addPath(landPath);
        }
    }
}


bool CProcessor::buildSimplifiedPolygonWkt(QList<SimplifiedWayEntry>& simplifiedWayList, QString& wkt)
{
    wkt = "MULTIPOLYGON(((";
    SimplifiedWayEntry swe;
    int innerRing = 0;
    bool firstLoop = true;
    for (int i = 0; i < simplifiedWayList.size(); i++)
    {
        swe = simplifiedWayList.at(i);
        if (swe.innerRingId > innerRing)
        {
            innerRing++;
            firstLoop = true;
            wkt.chop(1); //remove the trailing comma 
            wkt += "),(";
        }
        QString t = QString::fromStdString(swe.wkt);
        if (t.size() < 12)
            continue;
        t = t.mid(11, t.size() - 11 - 1); //Strips off LINESTRING( and closing bracket 

        //Remove the starting coordinates as these are already present from the previous wkt loop but only if not the first time around 
        if (firstLoop)
            firstLoop = false;
        else
        {
            int index = t.indexOf(',');
            if (index != -1)
                t = t.mid(index + 1);
        }

        //Add cleaned string to main result 
        wkt += t + ",";
    }

    //Finished so add on closing brackets after removing trailing zero
    wkt.chop(1);
    wkt += ")))";
    return true;
}