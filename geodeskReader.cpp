//geodeskReader.cpp
#include "geodeskReader.h"
#include "node.h"
#include "way.h"
#include "relation.h"
#include "processor.h"
#include "spatial.h"
#include "loggingCategories.h"
#include <QDebug>


//Set up logging category hierarchy
Q_LOGGING_CATEGORY(geodeskReaderManagement, "geodeskReader.management")

using namespace geodesk;

CGeodeskReader::CGeodeskReader( )
{
}

CGeodeskReader::~CGeodeskReader()
{
}

bool CGeodeskReader::readRelations(CProcessor* processor, int currentLayerId, bool writeCountries )
{
    //Check have key strings
    if (m_geodeskSource.isEmpty() || m_geodeskFilter.isEmpty())
    {
        qCCritical(geodeskReaderManagement) << "No Geodesk source or filter values. Check schedule.";
        return false;
    }

    //Create a bounding box 
    double west, south, east, north;
    getGeodeskArea(west, south, east, north);
    //Currently default value is 85 degrees north and south as bug in Geodesk does not do correct clamping when calculating lats to 90 degrees so could use Box::ofWorld() instead but not worried too much at mo
    Box filterBox = Box::ofWSEN(west, south, east, north);

    //Create features
    Features planet(m_geodeskSource.toUtf8().constData());
    Features planetFiltered = planet(m_geodeskFilter.toUtf8().constData());
    Features areaFeatures = planetFiltered(filterBox);

    //Iterate through and only process relations
    for (Feature f : areaFeatures)
    {
        if (!f.isRelation())
            continue;

        //Check if this is an excluded relation
        TagValue tv = f["name:en"];
        std::string stv = tv;
        if (isExcluded(stv))
            continue;

        //Create a new relation and add to map
        quint64 relationId = f.id();
        CRelation* relation = new CRelation(relationId);
        m_relationsMaster.insert(relationId, relation);

        //Iterate through the members 
        Features members = f.members();
        for (Feature member : members)
        {
            //Process depending on type and role 
            QString mType = member.typeName();
            if (mType == "way")
            {
                Q_ASSERT(member.isWay());
                CWay* way = nullptr;

                //Create new way unless already exists
                quint64 wayId = member.id();
                if (m_waysMaster.contains(wayId))
                {
                    way = m_waysMaster.value(wayId);
                    way->addParent(relationId);
                    continue;
                }
                way = new CWay(wayId);
                way->addParent(relationId);
                m_waysMaster.insert(wayId, way);

                //Get the way nodes  
                Features nodes = member.nodes();
                for (Feature node : nodes)
                {
                    Q_ASSERT(node.isNode());
                    quint64 nodeId = node.id();
                    double longitude = node.lon();
                    double latitude = node.lat();

                    CNode* pNode = nullptr;
                    pNode = getNode(nodeId, longitude, latitude );
                    Q_ASSERT(pNode);
                    pNode->addParent(wayId);
                    way->addNode(pNode);
                    way->getBoundingBox().Grow(longitude, latitude);
                }
                //Have all the nodes so have enough info to create a way member  which also grows the relation bounding box 
                QString role = QString::fromStdString(member.role());
                relation->addWayMember(way, role);
            }
            else if (mType == "node")
            {
                Q_ASSERT(member.isNode());
                quint64 nodeId = member.id();
                double longitude = member.lon();
                double latitude = member.lat();

                CNode* pNode = nullptr;
                pNode = getNode(nodeId, longitude, latitude);
                Q_ASSERT(pNode);
                //Do not want to set this node to have a parent 
                QString role = QString::fromStdString(member.role());
                relation->addNodeMember(pNode, role);
            }
        } //members loop 

        //Get tags for the feature / relation 
        Tags tags = f.tags();
        for (Tag t : tags)
        {
            QString key = QString::fromStdString(t.key());
            QString value = QString::fromStdString(t.value());
            relation->addTag(key, value);
        }

        //Relation is populated with tags, ways and the ways are populated with nodes 
        //Tell the relation which layer it is part of
        relation->addParent(currentLayerId);

        //Progress update 
        qCInfo(geodeskReaderManagement) << "Deducing geometries for " << relation->getName();

        //Process the relation
        bool ok = processor->processRelation(relation, currentLayerId, writeCountries );
        if (!ok)
        {
            qCWarning(geodeskReaderManagement) << "Failed to process geometries for Relation " << relation->getName() << "(" << relation->getId() << ")";;
            continue;
        }

        //Clean up  
        //Delete the maps
        deleteMasters();
    }

    //Update polygon areas
    bool ok = processor->updateAreas();
    if (!ok)
    {
        qCCritical(geodeskReaderManagement) << "Failed to update areas of new relations.";
        return false;
    }

    return true;
}

void CGeodeskReader::getGeodeskArea(double& minX, double& minY, double& maxX, double& maxY)
{
    //If no area filter set to entire world 
    if (m_geodeskAreaFilter.isEmpty())
    {
        minX = -180.0;
        minY = -85.0;
        maxX = 180.0;
        maxY = 85.0;
        qCInfo(geodeskReaderManagement) << "Geodesk area set to entire world.";
        return;
    }

    //Check if format is WSEN in numbers surrounded by parentheses 
    if (m_geodeskAreaFilter.at(0) == '('
        && m_geodeskAreaFilter.at(m_geodeskAreaFilter.size() - 1) == ')')
    {
        QString trimmed = m_geodeskAreaFilter.trimmed();
        trimmed = trimmed.remove('(');
        trimmed = trimmed.remove(')');

        //Split into parts
        QStringList parts = trimmed.split(',');
        if (parts.size() == 4)
        {
            minX = parts[0].trimmed().toDouble();
            minY = parts[1].trimmed().toDouble();
            maxX = parts[2].trimmed().toDouble();
            maxY = parts[3].trimmed().toDouble();
            qCInfo(geodeskReaderManagement) << "Geodesk area set to"" " << m_geodeskAreaFilter;
            return;
        }
        else
        {
            qCWarning(geodeskReaderManagement) << "Unable to interpret area" << m_geodeskAreaFilter << "so reverting to entire world.";
            minX = -180.0;
            minY = -85.0;
            maxX = 180.0;
            maxY = 85.0;
            return;
        }
    }

    //Create features based on an overpass style filter 
    Features planet(m_geodeskSource.toUtf8().constData());
    Features planetAreaFiltered = planet(m_geodeskAreaFilter.toUtf8().constData());

    //Have to iterate through each feature to grow a bounding box
    Box boundingArea;
    for (Feature f : planetAreaFiltered)
    {
        Box fBox = f.bounds();
        boundingArea.expandToIncludeSimple(fBox);
    }

    //Get the values
    minX = boundingArea.minLon();
    minY = boundingArea.minLat();
    maxX = boundingArea.maxLon();
    maxY = boundingArea.maxLat();
}

bool CGeodeskReader::isExcluded(std::string& relationName )
{
    bool exclude = false;

    //Cycle through excluded relations list 
    for (QString excludedRelation : m_excludedTags)
    {
        if (excludedRelation == relationName )
        {
            exclude = true;
            break;
        }
    }

    return exclude;
}

void CGeodeskReader::deleteMasters()
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

CNode* CGeodeskReader::getNode(quint64 nodeId, double longitude, double latitude)
{
    Q_ASSERT(nodeId > 0);
    CNode* pNode = nullptr;
    if (m_nodesMaster.contains(nodeId))
    {
        pNode = m_nodesMaster[nodeId];
    }
    else
    {
        QString sLongitude = QString::number(longitude, 'f', 7);
        QString sLatitude = QString::number(latitude, 'f', 7);
        pNode = new CNode(nodeId, sLongitude.toUtf8(), sLatitude.toUtf8());
        m_nodesMaster.emplace(nodeId, pNode);
    }

    return pNode;
}

void CGeodeskReader::setGeodeskSource(const QString& source)
{
    m_geodeskSource = source;
}

void CGeodeskReader::setGeodeskFilter(const QString& filter)
{
    m_geodeskFilter = filter;
}

void CGeodeskReader::setGeodeskArea(const QString& areaFilter)
{
    m_geodeskAreaFilter  = areaFilter;
}

void CGeodeskReader::clearFilters()
{
    m_geodeskSource = "";
    m_geodeskFilter = "";
    m_geodeskAreaFilter = "";
    m_excludedTags.clear();
}

bool CGeodeskReader::addExclusion(const QString& tag)
{
    //Strip any brackets and apostrophes 
    QString trimmed = tag.trimmed();
    QString lessBrackets = trimmed.mid(1, trimmed.length() - 2);

    //Split at the equals sign
    int firstEqualsIndex = lessBrackets.indexOf('=');

    // If an equals sign is found split the string
    if (firstEqualsIndex != -1)
    {
        QString key = lessBrackets.left(firstEqualsIndex).trimmed();
        if (key.at(0) == "'" && key.at(key.length() - 1) == "'")
            key = key.mid(1, key.size() - 2);
        QString value = lessBrackets.mid(firstEqualsIndex + 1).trimmed();
        if (value.at(0) == "'" && value.at(value.length() - 1) == "'")
            value = value.mid(1, value.size() - 2);
        m_excludedTags.append(value);
    }
else
    {
        qCWarning(geodeskReaderManagement) << "Invalid exclusion tag format:" << tag;
        return false;
    }


    return true;
}

bool CGeodeskReader::readWays(CSpatial* spatial, int currentLayerId)
{
    bool ok = true;

    //Get area to filter which either defaults to entire world or area of features returned by area filter 
    double minX, minY, maxX, maxY;

    getGeodeskArea(minX, minY, maxX, maxY);
    int divX = 36 * (maxX - minX) / 360;
    int         divY = 18 * (maxY - minY) / 180.0;
    if (divX == 0) divX = 1;
    if (divY == 0) divY = 1;

    qCInfo(geodeskReaderManagement) << "Getting ways from total area (WSEN)" << minX << "," << minY << "," << maxX << "," << maxY << "with" << divX << "x divisions and" << divY << "y divisions.";

    //Set up loops to collect data in smaller boxes 
    CBoundingBox worldBB;
    worldBB.setBoundingBox(minY, minX, maxY, maxX); //SWNE 
    for (int y = 0; y < divY; y++)
    {
        for (int x = 0; x < divX; x++)
        {
            double south = worldBB.south() + y * worldBB.height() / divY;
            double west = worldBB.west() + x * worldBB.width() / divX;
            double north = worldBB.south() + (y + 1) * worldBB.height() / divY;
            double east = worldBB.west() + (x + 1) * worldBB.width() / divX;

            qCInfo(geodeskReaderManagement) << "Getting ways in bounding box" << (y * divX + x) + 1 << "out of" << divX * divY;

            ok = readWaysInBoundingBox(south, west, north, east, spatial, currentLayerId);
            if (!ok)
            {
                qCWarning(geodeskReaderManagement) << "Problem getting ways from GeoDesk in bounding box" << south << "," << west << "," << north << "," << east;
            }
        }
    }

    return true;
}

bool CGeodeskReader::readWaysInBoundingBox(double south, double west, double north, double east, CSpatial* spatial, int currentLayerId)
{
    //Create a bounding box 
    Box filterBox = Box::ofWSEN(west, south, east, north);

    //Create features
    Features planet(m_geodeskSource.toUtf8().constData());
    Features planetFiltered = planet(m_geodeskFilter.toUtf8().constData());
    Features featuresInArea = planetFiltered(filterBox);

    //Iterate through and only process ways 
    for (Feature f : featuresInArea)
    {
        if (!f.isWay())
            continue;

        //Create a new way and add to map
        quint64 wayId = f.id();
        Q_ASSERT(!m_waysMaster.contains(wayId));
        CWay* way = new CWay(wayId);
        m_waysMaster.insert(wayId, way);

        //Get the way nodes  
        Features nodes = f.nodes();
        for (Feature node : nodes)
        {
            Q_ASSERT(node.isNode());
            quint64 nodeId = node.id();
            double longitude = node.lon();
            double latitude = node.lat();

            CNode* pNode = nullptr;
            pNode = getNode(nodeId, longitude, latitude);
            Q_ASSERT(pNode);
            pNode->addParent(wayId);
            way->addNode(pNode);
            way->getBoundingBox().Grow(longitude, latitude);
        } //nodes loop
    } //way loop 

    //The data set could be huge so all can do for now is store in database 
    bool ok = spatial->insertWays(m_waysMaster, currentLayerId);
    if (!ok)
    {
        qCCritical(geodeskReaderManagement) << "Failed to insert way data from geodesk.";
    }

    //Clean up
    //Delete the maps
    deleteMasters();

    return true;
}