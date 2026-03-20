//Conductor.cpp
#include "Conductor.h"
#include <QProcess>
#include <QSettings>
#include <QDebug>
#include "relation.h"
#include "way.h"
#include "node.h"
#include "osmElement.h"
#include "loggingCategories.h"
#include "DbManager.h"
#include "spatial.h"
#include "processor.h"
#include <geodesk/geodesk.h>
#include <iostream>
#include <sstream>
#include <iomanip>

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(ConductorProgress, "Conductor.progress")

using namespace geodesk;

CConductor::CConductor( QObject* parent )
    : QObject( parent )
{
    m_DbManager = new CDbManager(this);
    m_spatial = new CSpatial(m_DbManager, this);
  m_processor = new CProcessor(m_spatial , this );
  clearFilters();
  loadSettings();
}

CConductor::~CConductor()
{
}

bool CConductor::init(const QString& scheduleFilename)
{
    bool result = m_schedule.readInstructionFile(scheduleFilename);

    return result;
}

bool CConductor::executeSchedule()
{
    bool result = true;

    //Go through the schedule  and carry out the instructions
    QString action, detail;

    while (m_schedule.getNextInstruction(action, detail)
        && result)
    {
        //Set database name
        if (action == "database")
        {
            qCInfo(ConductorProgress) << "Creating or opening database" << detail;
            result = m_DbManager->createOpenDatabase( detail);
        }
        //read script file 
        else if (action == "readScript")
        {
            qCInfo(ConductorProgress) << "Reading script file:" << detail;
            result = m_DbManager->readScriptFile(detail);
        }
        //Layer info 
        else if (action == "layerName" )
        {
            qCInfo(ConductorProgress) << "Clearing layer filters.";
            clearFilters();
            m_newLayerName= detail;
        }
        else if (action == "layerGeometry")
        {
            m_newLayerGeometry = detail;
        }
        else if (action == "createLayer" && detail == "yes")
        {
            qCInfo(ConductorProgress) << "Creating new layer" << m_newLayerName << "containing" << m_newLayerGeometry;
            result = insertLayer(m_newLayerName, m_newLayerGeometry);
        }
        //Define a bounding box 
        else if (action == "boundingBox")
        {
            qCInfo(ConductorProgress) << "Setting bounding box: " << detail;
            result = m_OSMLoader.setBoundingBox(detail);
        }
        else if (action == "xDivisions")
        {
            qCInfo(ConductorProgress) << "Setting x divisions : " << detail;
            result = m_OSMLoader.setXDivisions(detail);
        }
        else if (action == "saveDownload")
        {
            qCInfo(ConductorProgress) << "Setting save download status: " << detail;
            result = m_OSMLoader.setSaveDownloads(detail);
        }
        else if (action == "yDivisions")
        {
            qCInfo(ConductorProgress) << "Setting y divisions : " << detail;
            result = m_OSMLoader.setYDivisions(detail);
        }
        else if (action == "requiredTag")
        {
            qCInfo(ConductorProgress) << "Adding required tag: " << detail;
            result = m_OSMLoader.appendRequiredTag(detail);
        }
        else if (action == "excludeByTag" )
        {
            qCInfo(ConductorProgress) << "Excluding by tag" << detail;
result =             m_geodeskReader.addExclusion(detail);
        }
        else if (action == "geodeskSource")
        {
            qCInfo(ConductorProgress) << "Setting geodesk source: " << detail;
            m_geodeskReader.setGeodeskSource(detail);
        }
        else if (action == "geodeskFilter")
        {
            qCInfo(ConductorProgress) << "Adding geodesk filter: " << detail;
            m_geodeskReader.setGeodeskFilter(detail);
        }
        else if (action == "geodeskAreaFilter")
        {
            qCInfo(ConductorProgress) << "Setting geodesk area filter: " << detail;
            m_geodeskReader.setGeodeskArea(detail);
        }
        else if (action == "step" && detail == "DownloadRelations" )
        {
            qCInfo(ConductorProgress) << "Downloading relation layer " << m_currentLayerId << m_currentLayerName;
            result = DownloadAndprocessRelations();
        }
        else if (action == "step" && detail == "DownloadWays")
        {
            qCInfo(ConductorProgress) << "Downloading Way layer " << m_currentLayerId << m_currentLayerName;
            result = DownloadAndprocessWays();
        }
        else if (action == "getGeodeskRelations" && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Getting relation data from GeoDesk planet for layer" << m_currentLayerId << m_currentLayerName;
            result = m_geodeskReader.readRelations(m_processor, m_currentLayerId, ifDoingCountryLayer() );

            //If doing islands they need associating with a territorial water to gauge the sovereign state
            if (result && ifDoingIslandLayer())
            {
                qCInfo(ConductorProgress) << "Associating islands to countries.";
                int islandLayerId = m_layers.getLayerId(m_islandLayerName);
                int territorialWatersLayerId = m_layers.getLayerId(m_territorialWatersLayerName);
                result = m_processor->associateIslandsToCountries(islandLayerId, territorialWatersLayerId );
            }
        }
        else if (action == "getGeodeskWays" && detail == "yes")
        {
            qCInfo(ConductorProgress) << "Getting way data from GeoDesk planet" << m_currentLayerId << m_currentLayerName;
            result = m_geodeskReader.readWays( m_spatial, m_currentLayerId);
        }
        else if (action == "removeTerritorialWaters" && detail == "yes")
        {
            qCInfo(ConductorProgress) << "Attempting to remove territorial waters from countries.";
            int countryLayerId = m_layers.getLayerId(m_CountryLayerName);
            int coastlineLayerId = m_layers.getLayerId( m_coastlineLayerName );
            int territorialWatersLayerId = m_layers.getLayerId( m_territorialWatersLayerName );
            result = m_processor->removeTerritorialWaters( countryLayerId, coastlineLayerId, territorialWatersLayerId);
        }
        else if (action == "DouglasPeukerTolerance")
        {
            qCInfo(ConductorProgress) << "Setting simplification tolerance to" << detail << "m.";
            m_processor->setSimplificationTolerance( detail );
            }
        else if (action == "column")
        {
            qCInfo(ConductorProgress) << "Simplification column is" << detail;
            m_processor->setSimplificationColumn(detail);
            }
        else if (action == "simplifyWaysAndPolygons"  && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Simplifying ways and polygons.";
            result = simplifyWaysAndPolygons();
        }
        else if (action == "zoomBand")
        {
            qCInfo(ConductorProgress) << "Polygon area visibility: zoom band <greater than >=" << detail;
            m_processor->setZoomBand(detail);
            }
        else if (action == "areaThreshold" )
        {
            qCInfo(ConductorProgress) << "Polygon area visibility: area threshold" << detail;
            m_processor->setAreaThreshold(detail);
            }
        else if (action == "updatePolygonVisibility" && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Updating polygon visibility.";
            m_processor->updatePolygonVisibility();
            }
            //preprocess links with contains relationship 
        else if (action == "preprocessLinks-contains" && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Prepprocessing links for polygons that contain other polygons.";
            result = m_spatial->preprocessLinksContains();
            }
            //preprocess links with borders relationship 
        else if (action == "preprocessLinks-borders")
        {
            qCInfo(ConductorProgress) << "Preprocessing links to add borders based on shared ways and no contains relationship.";
            result = m_spatial->preprocessLinksBorders();
            }
            //Work out polygons for countries from higher admin levels 
        else if (action == "deriveCountryPolygons" && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Deriving country polygons from higher admin levels.";
            result = m_processor->buildCountryPolygonsFromHigherAdminLevels(m_currentLayerId);
            }
        else if (action == "identifyBorders" )
        {
            qCInfo(ConductorProgress) << "Identifying bordering relations";
            bool maritime = ( detail == "maritime" ) ? true : false;
            result = m_spatial->identifyBorders(maritime );
        }
        else if (action == "setColours" and detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Assigning a colour index to each relation.";;
            result = assignColourIndex();
        }
        else if (action == "updatePolygonsWithLayersAndColours" && detail == "yes" )
        {
            qCInfo(ConductorProgress) << "Updating polygons with layer and colour index.";
            result = m_spatial->updatePolygonsWithLayersAndColours();
            }
        else if (action == "file" )
        {
            qCInfo(ConductorProgress) << "Reading from file " << detail;
            result = ProcessFromFile( "files/" + detail );
        }
        else if (action == "end")
        {
            qCInfo(ConductorProgress) << "Instructed to finish.";
            break;
        }
    }


    //If there was a failure the master lists might not be empty
    if (!result)
    {
        deleteMasters();
    }

    return result;
}

bool CConductor::DownloadAndprocessRelations()
{
    bool ok = false;

    //Loop through the sub bounding boxes in OSM loader
    //For each sub bounding box get the relations that could be downloaded 
    //Exclude relations already downloaded 
    //Then download each relation one at a time and put into database 
    m_OSMLoader.moveToFirstSubBoundingBox();
    while (m_OSMLoader.moveToNextSubBoundingBox())
    {
        QList<quint64> relationIDs;
        ok = m_OSMLoader.getRelationIDList(relationIDs);
        if (!ok)
        {
            qCWarning(ConductorProgress) << "Error encountered retrieving relation IDs from subboundingbox  so skipping" << m_OSMLoader.GetSubBoundingBoxAsString();
            continue;
        }

        //Remove relation IDs that have already been downloaded into database 
        ok = m_spatial->removeAlreadyProcessedRelationIDs(relationIDs);
        if (!ok)
        {
            qCWarning(ConductorProgress) << "Failed to remove already processed relation IDs for a sub bounding box";
            continue;
        }

        //Have a refined list of relations to get so now go ahead and get them in detail  and process 
        int counter = 1;
        for (int relationId : relationIDs)
        {
            //Status update
            qCInfo(ConductorProgress) << "Downloading relation " << counter++ << " of " << relationIDs.size() << " with ID " << relationId;

            ok = m_OSMLoader.downloadRelationNWR(relationId, m_nodesMaster, m_waysMaster, m_relationsMaster);
            if (!ok)
            {
                qWarning(ConductorProgress) << "Failed to  download full relation " << relationId;
                continue;
            }

            //The full relation data is in the relation master map 
            CRelation* relation = m_relationsMaster.value(relationId, nullptr);
            if (relation == nullptr)
            {
                qCWarning(ConductorProgress) << "Cannot retrieve the relation just created. Relation id:" << relationId;
                continue;
            }

            //Tell the relation which layer it is part of
            relation->addParent(m_currentLayerId);

            //Progress update 
            qCInfo(ConductorProgress) << "Deducing geometries for " << relation->getName();

            //Process the relation
            ok = m_processor->processRelation(relation, m_currentLayerId , ifDoingCountryLayer() );
            if (!ok)
            {
                qCWarning(ConductorProgress) << "Failed to process geometries for Relation " << relation->getName() << "(" << relation->getId() << ")";;
                continue;
            }

        } //For each relation 
        //Clear out master list ready for next bounding box
        deleteMasters();
    } //bounding box loop 

    //Update the areas for the polygons and then the relations 
    m_spatial->updateAreas();

    return ok;
}

bool CConductor::DownloadAndprocessWays()
{
    bool ok = false;

    //Loop through the sub bounding boxes in OSM loader
    //For each sub bounding box download all the ways that match 
    int counter = 1;
    m_OSMLoader.moveToFirstSubBoundingBox();
    while (m_OSMLoader.moveToNextSubBoundingBox() )
    {
        qCInfo(ConductorProgress) << "Downloading ways in subarea " << counter++ << ":" << m_OSMLoader.GetSubBoundingBoxAsString();
        ok = m_OSMLoader.downloadWaysNW(m_nodesMaster, m_waysMaster);
        if (!ok)
        {
            qWarning(ConductorProgress) << "Failed to  download ways  in subboundingbox" << m_OSMLoader.GetSubBoundingBoxAsString();
            continue;
        }

        //The data set could be huge so all can do for now is store in database 
        ok = m_spatial->insertWays(m_waysMaster, m_currentLayerId);
        if (!ok)
        {
            qCCritical(ConductorProgress) << "Failed to insert way data from subboundingbox" << m_OSMLoader.GetSubBoundingBoxAsString();                continue;
            continue;
        }

        //Tidy up nodes and ways for the relations just done
        deleteMasters();
    }

    return ok;
}

bool CConductor::ProcessFromFile(const QString& filename)
{
    bool ok = false;

    ok = m_OSMLoader.loadRelationsNWRFromFile(filename, m_nodesMaster, m_waysMaster, m_relationsMaster);
    if (!ok)
    {
        qCCritical(ConductorProgress) << "Error loading relations from file" << filename;
        return false;
    }

    //Iterate through relations in relation master map
    for (CRelation* relation : m_relationsMaster)
    {
        //Set the relation layer
        relation->addParent(m_currentLayerId);

        //Process the relation
        ok = m_processor->processRelation(relation, m_currentLayerId, ifDoingCountryLayer() );
        if (!ok)
        {
            qCCritical(ConductorProgress) << "Failed to process geometries for Relation " << relation->getName() << "(" << relation->getId() << ")";;
            return false;
        }
    }

    //Tidy master lists
    deleteMasters();

    return true;
}

void CConductor::loadSettings()
{
    QSettings settings("config.ini", QSettings::IniFormat);

    //Load default layer names writing back immediately so INI file is populated if it did not exist before 
    settings.beginGroup("BaseLayers");
    m_CountryLayerName = settings.value("countryLayerName", "countries").toString();
    settings.setValue( "countryLayerName", m_CountryLayerName);
    m_islandLayerName = settings.value( "islandLayerName", "islands" ).toString();
    settings.setValue( "islandLayerName", m_islandLayerName);
    m_coastlineLayerName = settings.value("coastlineLayerName", "coastlines").toString();
    settings.setValue("coastlineLayerName", m_coastlineLayerName);
    m_territorialWatersLayerName = settings.value("territorialWatersLayerName", "territorialWaters").toString();
    settings.setValue("territorialWatersLayerName", m_territorialWatersLayerName);

    settings.endGroup();
}

bool CConductor::insertLayer(const QString& layerName, const QString& layerGeometry )
{
    //Check there are at least populated strings 
    if (layerName.isEmpty() || layerGeometry.isEmpty())
    {
        qCCritical(ConductorProgress) << "Empty layer name and/or geometry.";
        return false;
    }

    //Get the geometry id for this layer
    int geometryId = -1;
    bool ok = m_spatial->selectGeometryId(layerGeometry, geometryId);
    if (!ok )
        return false;

    if (geometryId == -1)
    {
        qCCritical(ConductorProgress) << "Failed to match geometry category" << layerGeometry << "with database.";
        return false;
    }

    //Insert the new layer
    ok = m_spatial->InsertLayer(layerName, geometryId );
    if (!ok)
        return false;

    //Then load all layers back into the layers list
    ok = m_spatial->readLayerNames(m_layers);
    if (!ok)
        return false;

    //Get the id
    m_currentLayerId = m_layers.getLayerId( layerName );

    return true;
}

void CConductor::deleteMasters()
{
    //Go through each master list and delete memory allocated
    //Relations first, then ways then nodes
    for (CRelation* relation : m_relationsMaster)
    {
        delete relation;
    }
    m_relationsMaster.clear();

    for (CWay* way : m_waysMaster)
    {
        delete way;
    }
    m_waysMaster.clear();

    for (auto const& [key, node] : m_nodesMaster)
    {
        delete node;
    }
    m_nodesMaster.clear();
}

bool CConductor::assignColourIndex()
{
    // Create a QProcess object
    QProcess process;

    // Set up the command to run 
    QString program = "python";
    QStringList arguments;
    arguments << "python/ColourBorderingCountries.py"; 
    arguments << m_DbManager->getDatabaseName();

    // Start the process
    process.start(program, arguments);

    // Wait for the process to finish
    if (!process.waitForFinished()) 
    {
        qCWarning(ConductorProgress) << "Python script for colour indexes failed.";
        return false;
    }

    // Read the output
    QByteArray output = process.readAllStandardOutput();
    qCInfo( ConductorProgress ) << "Python colour index Output:\n" << output;

    QByteArray error = process.readAllStandardError();
    if (!error.isEmpty()) 
    {
        qCWarning( ConductorProgress ) << "Script Error:\n" << error;
        return false;
    }

    return true;
}

void CConductor::clearFilters()
{
    m_currentLayerId = 0;
    m_currentLayerName = "";
    m_geodeskReader.clearFilters();
    m_OSMLoader.clearRequiredTags();
}

bool CConductor::ifDoingCountryLayer()
{
    //Special action needed if doing the country layer
    bool doingCountries = false;
    if (m_layers.getLayerId( m_CountryLayerName ) == m_currentLayerId )
        doingCountries = true;
    return doingCountries;
}

bool CConductor::ifDoingIslandLayer()
{
    //Special action needed if doing the island layer
    bool doingIslands = false;
    if (m_layers.getLayerId(m_islandLayerName) == m_currentLayerId)
        doingIslands= true;
    return doingIslands;
}

bool CConductor::simplifyWaysAndPolygons()
    {
    //First disable indexes 
    bool ok = true; // m_DbManager->executeSqlFile("../scripts/batch/disableWayPolygonIndexes.sql");
    //No checking of return result because do not mind if it failed 

    //qCInfo(spatialManagement) << "Indexes dropped and disabled.  Now simplifying.";

    //Do the work
    ok = m_processor->simplifyWaysAndPolygons();
    if (!ok)
        return false;
    qCInfo(ConductorProgress) << "Simplification completed. Restoring indexes.";

    //Recreate the indexes
    //ok = m_DbManager->executeSqlFile("../scripts/batch/CreateWayPolygonIndexes.sql");
    //again do not mind if these fail 
    //qCInfo(ConductorProgress) << "Indexes restored successfully.";

        return true;
    }