//OSMLoader.cpp
    
#include "OSMLoader.h"
#include <QCoreApplication>
#include <QTimer.h>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QEventLoop>
#include <QFile>
#include <QDebug>
#include "baseosmobject.h"
#include "node.h"
#include "way.h"
#include "relation.h"
#include "curl.h"
#include "loggingCategories.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(OSMLoaderManagement, "OSMLoader.management")


//Used by curl as callback for growing memory as XML is returned
struct MemoryStruct {
    char* memory;
    size_t size;
};

//Callback function for curl to grow memory when a new data packet is received 
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;
    mem->memory = (CHAR*)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) 
    {
        /* out of memory */
        qDebug() << "not enough memory (realloc returned NULL)";
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}



COSMLoader::COSMLoader( QObject* parent )
    : QObject( parent),
    m_networkManager(new QNetworkAccessManager(this))
{
    //Initialize curl
    curl_global_init(CURL_GLOBAL_ALL);

    m_saveDownloadToFile = false;
}

COSMLoader::~COSMLoader()
{
    // we are done with libcurl, so clean it up 
    curl_global_cleanup();
}

bool COSMLoader::setBoundingBox(const QString& boundingBox)
{
    //Three options on how bounding box is set
    if (boundingBox == "all")
    {
        m_strBoundingBox = "(-90,-180,90,180)";
        bool ok = m_BoundingBox.setBoundingBoxFromString( m_strBoundingBox);
        if (!ok)
        {
            qCWarning(OSMLoaderManagement) << "Failed to set bounding box to 'all'.";
            return false;
        }
    }
    else if (boundingBox.length() > 0 && boundingBox.at(0) == '(')
    {
        //Format is normal (south,west,north,east) format 
        bool ok = m_BoundingBox.setBoundingBoxFromString(boundingBox);
        if (!ok)
        {
            qCWarning(OSMLoaderManagement) << "Failed to set bounding box to " << boundingBox;
            return false;
        }
        m_strBoundingBox = boundingBox;
    }
    else
    {
        //Assume a country name 
        bool ok = setBoundingBoxFromCountry(boundingBox);
        if (!ok)
        {
            qCWarning(OSMLoaderManagement) << "Failed to set bounding box from country " << boundingBox;
            return false;
        }
    }
    return true;
}

bool COSMLoader::setBoundingBoxFromCountry(const QString& countryName)
{
    if (countryName.isEmpty())
        return false;


    // Construct the Overpass QL query.
    //QString query = "[out:xml];"
    QString query =  "relation[\"admin_level\"=\"2\"][\"name:en\"~\"^" + countryName + "$\"];"
        "out%20bb;";

    // Define timeout (e.g., 90 seconds)
    const int TIMEOUT_MS = 90000;
    QString errorMsg;

    // Use the reusable method
    //QByteArray xmlData = executeQuerySync(query, TIMEOUT_MS, errorMsg);
    QByteArray xmlData = makeCurlCall(query, TIMEOUT_MS, errorMsg);

    //Save downloaded data to file
    if (m_saveDownloadToFile)
        writeQueryResponseToFile(xmlData, "roBB.txt");

    // Use QXmlStreamReader to parse the XML.
    QXmlStreamReader xmlReader(xmlData);

    bool boundingBoxFound = false;

    // Iterate through the XML elements
    while (!xmlReader.atEnd() && !xmlReader.hasError()) 
    {
        QXmlStreamReader::TokenType token = xmlReader.readNext();

        // Look for the "bounds" element.
        if (token == QXmlStreamReader::StartElement && xmlReader.name() == "bounds") 
        {
            // Extract the attributes.
            QXmlStreamAttributes attributes = xmlReader.attributes();
            if (attributes.hasAttribute("minlat") && attributes.hasAttribute("minlon") &&
                attributes.hasAttribute("maxlat") && attributes.hasAttribute("maxlon")) 
            {
                m_BoundingBox.setBoundingBox(
                    attributes.value("minlat").toDouble(),
                    attributes.value("minlon").toDouble(),
                    attributes.value("maxlat").toDouble(),
                    attributes.value("maxlon").toDouble()
                );
                boundingBoxFound = true;
                break; // Found the bounding box  so can stop
            } //Has correct attributes 
        } //Is the bounding box token 
    } //No error reading XML and not at end 

// Check for parsing errors and if the bounding box was found.
    if (xmlReader.hasError()) 
    {
        qCWarning(OSMLoaderManagement) << "XML parsing error:" << xmlReader.errorString();
        m_BoundingBox.reset();
        return false;
    }
    else 
    {
        return true;
    }
}

bool COSMLoader::setXDivisions(const QString& xDivision)
{
    if (xDivision.isEmpty())
    {
        qWarning(OSMLoaderManagement) << "x division of bounding box is empty.";
        m_xDivisions = 0;
        return false;
    }

    bool ok = false;
    m_xDivisions= xDivision.toInt(&ok);
    if (!ok)
    {
        qCWarning(OSMLoaderManagement) << "Unable to convert x division of bounding box to a number: " << xDivision;
        m_xDivisions = 0;
        return false;
    }
    return true;
}

bool COSMLoader::setYDivisions(const QString& yDivision)
{
    if (yDivision.isEmpty())
    {
        qWarning(OSMLoaderManagement) << "x division of bounding box is empty.";
        m_yDivisions = 0;
        return false;
    }

    bool ok = false;
    m_yDivisions  = yDivision.toInt(&ok);
    if (!ok)
    {
        qCWarning(OSMLoaderManagement) << "Unable to convert x division of bounding box to a number: " << yDivision;
        m_yDivisions = 0;
        return false;
    }
    return true;
}

void COSMLoader::clearRequiredTags()
{
    m_RequiredTags.clear();
}

bool COSMLoader::appendRequiredTag(const QString& requiredTag)
{
    //Check tag is not empty
    if (requiredTag.isEmpty())
    {
        qCWarning(OSMLoaderManagement) << "Required tag is empty.";
        return false;
    }

    //Check starts and ends with square brackets
    if (requiredTag.at(0) != '[' && requiredTag.at(requiredTag.length() - 1) != ']')
    {
        qCWarning(OSMLoaderManagement) << "Required tag does not start and end with square brackets: " << requiredTag;
        return false;
    }

    m_RequiredTags += requiredTag;

    return true;
}


void COSMLoader::moveToFirstSubBoundingBox()
{
    //Moving to minus one is a bit odd looking but it is because the moveNext method increments the counter and needs to have the current sub bb index for correct generation of the subboundingbox as a string 
    m_subBBCounter = -1;
}

bool COSMLoader::haveSubBoundingBox()
{
    return m_subBBCounter < m_xDivisions * m_yDivisions;
}


bool COSMLoader::moveToNextSubBoundingBox()
{
    return ++m_subBBCounter < m_xDivisions * m_yDivisions;
}

bool COSMLoader::getRelationIDList(QList<quint64>& relationIDs)
{
    // Clear the list to ensure it's empty before populating it
    relationIDs.clear();

    // Step 1: Construct the Overpass QL query string
    QString subBB = GetSubBoundingBoxAsString();

    // Begin the query with the settings
    // QString query = QString("[out:xml];"); 

    // Add the relation query part with the bounding box.
    QString query =  "relation" + subBB + m_RequiredTags;

    // Add the final output command to get IDs.
    query += ";out%20ids;";

    // Define timeout (e.g., 90 seconds)
    const int TIMEOUT_MS = 90000;
    QString errorMsg;

    // Use the reusable method
    //QByteArray xmlData = executeQuerySync(query, TIMEOUT_MS, errorMsg);
    QByteArray xmlData = makeCurlCall(query, TIMEOUT_MS, errorMsg);

    //Save downloaded data to file
    if (m_saveDownloadToFile)
        writeQueryResponseToFile(xmlData, "roRID.txt");


    // Parse the XML to extract relation IDs
    QXmlStreamReader xmlReader(xmlData);
    while (!xmlReader.atEnd() && !xmlReader.hasError()) 
    {
        QXmlStreamReader::TokenType token = xmlReader.readNext();

        if (token == QXmlStreamReader::StartElement && xmlReader.name() == "relation") 
        {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            if (attributes.hasAttribute("id")) 
            {
                quint64 id = attributes.value("id").toLongLong();
                relationIDs.append(id);
            } //Attribute is of type id 
        } //is a relation token 
    } // reading XML without error and not at end 

    if (xmlReader.hasError()) 
    {
        qCCritical( OSMLoaderManagement) << "XML parsing error during relation ID download:" << xmlReader.errorString();
        relationIDs.clear(); // Return an empty vector on error.
    }

    qCInfo( OSMLoaderManagement) << "Found" << relationIDs.size() << "relation IDs in bounding box " << m_subBBCounter << " out of " << m_xDivisions* m_yDivisions << ". boundingBox=" << subBB;
    
    return true;
}

QString COSMLoader::GetSubBoundingBoxAsString()
{
    QString subBB;

    //The sub boxes move from bottom left to top right scanning horizontally then upwards 
    int x = m_subBBCounter % m_xDivisions;
    int y = int(m_subBBCounter / m_xDivisions);

    //Can work out coordinates from this
    double south = m_BoundingBox.south() + y * m_BoundingBox.height() / m_yDivisions;
    double west = m_BoundingBox.west() + x * m_BoundingBox.width() / m_xDivisions;
    double north = m_BoundingBox.south() + (y + 1) * m_BoundingBox.height() / m_yDivisions;
    double east = m_BoundingBox.west() + (x + 1) * m_BoundingBox.width() / m_xDivisions;

    //Construct string
    subBB = QString("(%1,%2,%3,%4)").arg(
        QString::number(south, 'f', 8),
        QString::number(west, 'f', 8),
        QString::number(north, 'f', 8),
        QString::number(east, 'f', 8)
    );

    return subBB;
}

void COSMLoader::writeQueryResponseToFile(const QByteArray& data, const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(data);
        file.close();
        qCInfo(OSMLoaderManagement) << "Successfully wrote Overpass response to file:" << filePath;
    }
    else {
        qCCritical(OSMLoaderManagement) << "Could not open file for writing:" << filePath << "Error:" << file.errorString();
    }
}

bool COSMLoader::downloadRelationNWR(quint64 relationID, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster)
{
    //Construct the query to get nodes first, then ways, then relation details. Include bounding boxes 
    //QString query = QString("[out:xml];" // [timeout:90] ; "
    QString query =  QString( "(relation(%1);" // Get the parent relation
        "nw(r);" // Get ways that are members of the relation and their nodes 
        "node(w););" //nodes part of the way 
        "out%20bb;"  // inclue bounding box for relation 
    ).arg(relationID);

    // Define timeout (e.g., 90 seconds)
    const int TIMEOUT_MS = 90000;
    QString errorMsg;

    // Construct the full network call and execute it 
    //QByteArray xmlData = executeQuerySync(query, TIMEOUT_MS, errorMsg);
    QByteArray xmlData = makeCurlCall(query, TIMEOUT_MS, errorMsg);

    //Save downloaded data to file
    if (m_saveDownloadToFile)
        writeQueryResponseToFile(xmlData, "roNWR.txt");

    // Parse the XML data
    bool success = extractFullNWR(xmlData, rNodesMaster, rWaysMaster, rRelationsMaster);

    return success;
}

/*
bool COSMLoader::downloadRelationNWR(quint64 relationID, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster)
{
    //Construct the query to get nodes first, then ways, then relation details. Include bounding boxes 
    // QString query = QString("[out:xml];" // [timeout:90] ; "
    QString query =          QString( "(relation(%1);" // Get the parent relation
        "nw(r);" // Get ways that are members of the relation and their nodes 
        "node(w););" //nodes part of the way 
        "out%20bb;"  // inclue bounding box for relation 
    ).arg(relationID);


    // Prepare and send the synchronous POST request
    QUrl url("https://overpass-api.de/api/interpreter");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery postData;
    postData.addQueryItem("data", query);

    QNetworkReply* reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    // Use QEventLoop to wait for the reply.
    //QEventLoop loop;
    //connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    //loop.exec();

    if (!reply->waitForFinished(60000))
    {
    }
    // Process the response
    if (reply->error() != QNetworkReply::NoError)
    {
        qCCritical(OSMLoaderManagement) << "Network error during relation ID(" << relationID << ") download:" << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QByteArray xmlData = reply->readAll();
    reply->deleteLater();

    //Save downloaded data to file
    if( m_saveDownloadToFile )
        writeQueryResponseToFile(xmlData, "responseOutput.txt");

    // Parse the XML data
    bool success = extractFullNWR(xmlData, rNodesMaster, rWaysMaster, rRelationsMaster);

    return success;
}
*/
bool COSMLoader::loadRelationsNWRFromFile(const QString& filePath, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCCritical(OSMLoaderManagement) << "Failed to open file for reading:" << filePath;
        return false;
    }

    QByteArray xmlData = file.readAll();
    file.close();

    // Check for empty or corrupted file data
    if (xmlData.isEmpty())
    {
        qCCritical(OSMLoaderManagement) << "File is empty or could not be read:" << filePath;
        return false;
    }

    // Parse the XML data
    bool success = extractFullNWR(xmlData, rNodesMaster, rWaysMaster, rRelationsMaster);

    if (!success)
    {
        qCCritical(OSMLoaderManagement) << "Failed to parse XML data from file:" << filePath;
    }

    return success;
}

bool COSMLoader::extractFullNWR(QByteArray& xmlData, NodeMap& rNodesMaster, WayMap& rWaysMaster,RelationMap& rRelationsMaster)
{
    QXmlStreamReader xmlReader(xmlData);


    // Read the opening <osm> tag first.
    if (!xmlReader.readNextStartElement() || xmlReader.name() != "osm")
    {
        qCCritical(OSMLoaderManagement) << "XML file does not start with an <osm> element.";
        return false;
    }

    // Data is required to have all nodes first, then all ways and then relations last 
    while (xmlReader.readNextStartElement())
    {
        if (xmlReader.name() == "node")
        {
            if (!extractNode(xmlReader, rNodesMaster)) 
            {
                return false;
            }
        }
        else if (xmlReader.name() == "way")
        {
            if (!extractWay(xmlReader, rNodesMaster, rWaysMaster)) 
            {
                return false;
            }
        }
        else if (xmlReader.name() == "relation")
        {
            if (!extractRelation(xmlReader, rNodesMaster, rWaysMaster, rRelationsMaster)) 
            {
                return false;
            }
        }
        else
        {
            // Skip over any other top-level elements like <osm>, <note>, and <meta>
            xmlReader.skipCurrentElement();
        }
    }

    if (xmlReader.hasError())
    {
        qCCritical(OSMLoaderManagement) << "XML parsing error:" << xmlReader.errorString();
        return false;
    }

    return true;
}

bool COSMLoader::extractNode(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster)
{
    // Make sure we are at a "node" element
    if (xmlReader.name() != "node")
    {
        qCCritical(OSMLoaderManagement) << "extractNode called on non-node element:" << xmlReader.name();
        return false;
    }

    // Read the attributes for the node
    QXmlStreamAttributes attributes = xmlReader.attributes();
    bool idOk;
    quint64 id = attributes.value("id").toLongLong(&idOk);
    QString lat = attributes.value("lat").toString();
    QString lon = attributes.value("lon").toString();

    // Validate the core attributes (ID, latitude, longitude)
    if (!idOk || lat.isEmpty() || lon.isEmpty())
    {
        qCCritical(OSMLoaderManagement) << "Missing or invalid core attribute for node element.";
        return false;
    }

    // Check if the node already exists in the master list
    if (rNodesMaster.contains(id))
    {
        // If it exists, skip the element and its children and return.
        xmlReader.skipCurrentElement();
        if (xmlReader.hasError())
        {
            qCCritical(OSMLoaderManagement) << "XML parsing error while skipping node:" << xmlReader.errorString();
            return false;
        }
        return true; // Correctly returns here after skipping a duplicate
    }

    // Create a new CNode object and add to the master list
    CNode* node = new CNode(id, lon.toUtf8(), lat.toUtf8() );
    rNodesMaster.emplace(id, node);

    // Process all child elements (tags) of the new node
    while (xmlReader.readNextStartElement() && !xmlReader.hasError() )
    {
        if (xmlReader.name() == "tag")
        {
            bool tagOk = extractTag(xmlReader, node );
            if (!tagOk)
            {
                qCCritical(OSMLoaderManagement) << "Failed to extract tag for node" << id;
                return false;
            }
            //Skip current tag to move reader on
            xmlReader.skipCurrentElement();
        }
        else
        {
            // Skip any other unknown children
            xmlReader.skipCurrentElement();
        }
    }

    // The reader is now at the closing </node> tag. Check for final errors.
    if (xmlReader.hasError())
    {
        qCCritical(OSMLoaderManagement) << "XML parsing error after finishing node" << id << ":" << xmlReader.errorString();
        return false;
    }

    return true;
}

bool COSMLoader::extractWay(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster, WayMap& rWaysMaster)
{
    // Make sure we are at a "way" element
    if (xmlReader.name() != "way")
    {
        qCCritical(OSMLoaderManagement) << "extractWays called on non-way element:" << xmlReader.name();
        return false;
    }

    // Read the id attribute for the way
    QXmlStreamAttributes attributes = xmlReader.attributes();
    bool idOk;
    quint64 id = attributes.value("id").toLongLong(&idOk);
    if (!idOk)
    {
        qCCritical(OSMLoaderManagement) << "Unable to read id attribute when reading a way.";
        return false;
    }

    // Check if the way already exists in the master list
    if (rWaysMaster.contains(id))
    {
        // Skip this way element 
        xmlReader.skipCurrentElement();
        if (xmlReader.hasError())
        {
            qCCritical(OSMLoaderManagement) << "XML parsing error while skipping way:" << xmlReader.errorString();
            return false;
        }
        return true;
    }

    // Create a new CWay object
    CWay* way = new CWay(id);
    rWaysMaster.insert(id, way);

    // Read the child elements
    while (xmlReader.readNextStartElement() && !xmlReader.hasError() )
    {
        if (xmlReader.name() == "nd")
        {
            // Extract the node reference ID
            quint64 nodeId = xmlReader.attributes().value("ref").toLongLong();
            if (rNodesMaster.contains(nodeId))
            {
                // Get the node from the master list and add it to the way
                CNode* node = rNodesMaster[ nodeId ];
                way->addNode(node);

                xmlReader.skipCurrentElement();
            }
            else
            {
                //Node not found which is a critical error
                qCCritical(OSMLoaderManagement) << "Way " << id << "has node " << nodeId << "not present in master list";
                return false;
            }
        }
        else if (xmlReader.name() == "tag")
        {
            // Call the helper method to extract the tag
            bool tagOK = extractTag(xmlReader, way );
            if (!tagOK)
            {
                qCCritical(OSMLoaderManagement) << "Failed extracting tag from way" << id;
                return false;
            }
            xmlReader.skipCurrentElement();
        }
        else if (xmlReader.name() == "bounds")
        {
            double minlat, minlon, maxlat, maxlon;
            if (extractBoundingBox(xmlReader, minlat, minlon, maxlat, maxlon))
            {
                way->setBoundingBox(minlat, minlon, maxlat, maxlon);
            }
            else
            {
                qCCritical(OSMLoaderManagement) << "Error extracting bounding box from way" << id;
                return false;
            }

            // The reader is now inside the <bounds> element. Skip it.
            xmlReader.skipCurrentElement();
        }
        else
        {
            // Skip any other unknown elements
            xmlReader.skipCurrentElement();
        }
    }

    //Have loaded the way so put the back pointers in for the first and last nodes
    CNode* firstNode = way->getFirstNode();
    CNode* lastNode = way->getLastNode();
    firstNode->addParent(way->getId());
    lastNode->addParent(way->getId());

        // Check for errors after the loop
    if (xmlReader.hasError())
    {
        qCCritical(OSMLoaderManagement) << "XML parsing error while processing way " << id << "children:" << xmlReader.errorString();
        return false;
    }

    return true;
}

bool COSMLoader::extractRelation(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster)
{
    // Make sure we are at a "relation" element
    if (xmlReader.name() != "relation")
    {
        qCCritical(OSMLoaderManagement) << "extractRelations called on non-relation element:" << xmlReader.name();
        return false;
    }

    // Read the attributes for the relation
    QXmlStreamAttributes attributes = xmlReader.attributes();
    bool idOk;
    quint64 id = attributes.value("id").toLongLong(&idOk);

    // Validate the attribute
    if (!idOk )
    {
        qCCritical(OSMLoaderManagement) << "Missing or invalid attribute for relation element.";
        return false;
    }

    // Check if the relation already exists in the master list
    if (rRelationsMaster.contains(id))
    {
        // Skip processing its children if it's a duplicate
        xmlReader.skipCurrentElement();
        if (xmlReader.hasError())
        {
            qCCritical(OSMLoaderManagement) << "XML parsing error while processing relation " << id << "children:" << xmlReader.errorString();
            return false;
        }
        return true;
    }

    // Create a new relation object
    CRelation* relation = new CRelation( id );
    rRelationsMaster.insert(id, relation);

    // Read the child elements
    while (xmlReader.readNextStartElement() && !xmlReader.hasError() )
    {
        if (xmlReader.name() == "member")
        {
            attributes = xmlReader.attributes();
            quint64 refId = attributes.value("ref").toLongLong();
            QString type = attributes.value("type").toString();
            QString role = attributes.value("role").toString();

            if (type == "node")
            {
                if (rNodesMaster.contains(refId))
                {
                    relation->addNodeMember(rNodesMaster[ refId], role);
                }
            }
            else if (type == "way")
            {
                if (rWaysMaster.contains(refId))
                {
                    CWay* way = rWaysMaster.value(refId);
                    relation->addWayMember(way, role);
                    way->addParent( id );
                }
            }
            else if (type == "relation")
            {
                relation->addRelationMember(refId, role);
            }
            xmlReader.skipCurrentElement();
        }
        else if (xmlReader.name() == "tag")
        {
            // Call the helper method to extract all tags
            extractTag(xmlReader, relation );
            xmlReader.skipCurrentElement();
        }
        else if (xmlReader.name() == "bounds")
        {
            double minlat, minlon, maxlat, maxlon;
            if (extractBoundingBox(xmlReader, minlat, minlon, maxlat, maxlon))
            {
                relation->setBoundingBox(minlat, minlon, maxlat, maxlon);
            }
            else
            {
                qCCritical(OSMLoaderManagement) << "Error extracting bounding box from relation " << id;
                return false;
            }

            // The reader is now inside the <bounds> element. Skip it.
            xmlReader.skipCurrentElement();
        }
        else
        {
            // Skip any other unknown elements
            xmlReader.skipCurrentElement();
        }
    }

    // Check for errors again after the loop
    if (xmlReader.hasError())
    {
        qCCritical(OSMLoaderManagement) << "XML parsing error while processing relation:" << xmlReader.errorString();
        return false;
    }

    return true;
}


bool COSMLoader::extractTag(QXmlStreamReader& xmlReader, CBaseOSMObject* pObject)
{
    if (xmlReader.name() != "tag")
    {
        //Not at a tag 
        return false;
    }

    //Get key and value attributes for the tag 
    QXmlStreamAttributes attributes = xmlReader.attributes();
    QString key = attributes.value("k").toString();
    QString value = attributes.value("v").toString();

    //Check they are valid attributes 
    if (!key.isEmpty() && !value.isEmpty())
    {
        pObject->addTag(key, value);
        return true;
    }
    else
    {
        //Unable to read tag
        return false;
    }
}

bool COSMLoader::extractBoundingBox(QXmlStreamReader& xmlReader, double& minlat, double& minlon, double& maxlat, double& maxlon)
{
    //Assume at bounding box element 
    QXmlStreamAttributes bbAttributes = xmlReader.attributes();
    bool minLatOK, minLonOK, maxLatOK, maxLonOK;
    minlat = bbAttributes.value("minlat").toDouble(&minLatOK);
    minlon = bbAttributes.value("minlon").toDouble(&minLonOK);
    maxlat = bbAttributes.value("maxlat").toDouble(&maxLatOK);
    maxlon = bbAttributes.value("maxlon").toDouble(&maxLonOK);

    if (!minLatOK || !minLonOK || !maxLatOK || !maxLonOK)
    {
        //return so parent method can give a richer error 
        return false;
    }
    return true;
}

bool COSMLoader::downloadWaysNW(NodeMap& rNodesMaster, WayMap& rWaysMaster)
{
    //Construct the query to get nodes first and  then ways. Include bounding boxes 
    //QString query = QString("[out:xml];" 
    QString query = QString( "(way%1" // ways matching required tags 
        "%2;" // bounding box 
        "node(w););" //nodes part of the way 
        "out%20bb;"  // inclue bounding box for ways 
    ).arg(m_RequiredTags)
        .arg(GetSubBoundingBoxAsString());
    // Define timeout (e.g., 90 seconds)
    const int TIMEOUT_MS = 90000;
    QString errorMsg;

    // Use the reusable method
    //QByteArray xmlData = executeQuerySync(query, TIMEOUT_MS, errorMsg);
    QByteArray xmlData = makeCurlCall(query, TIMEOUT_MS, errorMsg);

    //Save downloaded data to file
    if (m_saveDownloadToFile)
        writeQueryResponseToFile(xmlData, "roWays.txt");

    // Parse the XML data
    bool success = extractFullNW(xmlData, rNodesMaster, rWaysMaster);

    return success;
}

bool COSMLoader::extractFullNW(QByteArray& xmlData, NodeMap& rNodesMaster, WayMap& rWaysMaster)
{
    QXmlStreamReader xmlReader(xmlData);


    // Read the opening <osm> tag first.
    if (!xmlReader.readNextStartElement() || xmlReader.name() != "osm")
    {
        qCCritical(OSMLoaderManagement) << "XML file does not start with an <osm> element.";
        return false;
    }

    // Data is required to have all nodes first and then all ways 
    while (xmlReader.readNextStartElement())
    {
        if (xmlReader.name() == "node")
        {
            if (!extractNode(xmlReader, rNodesMaster))
            {
                return false;
            }
        }
        else if (xmlReader.name() == "way")
        {
            if (!extractWay(xmlReader, rNodesMaster, rWaysMaster))
            {
                return false;
            }
        }
        else
        {
            // Skip over any other top-level elements like <osm>, <note>, and <meta>
            xmlReader.skipCurrentElement();
        }
    }

    if (xmlReader.hasError())
    {
        qCCritical(OSMLoaderManagement) << "XML parsing error:" << xmlReader.errorString();
        return false;
    }

    return true;
}

bool COSMLoader::setSaveDownloads(const QString& status)
{
    //Determines whether the relation download saves the downloaded data to a file 
    bool ok = false;
    if (status == "yes")
    {
        m_saveDownloadToFile = true;
        ok = true;
    }
    else if (status == "no")
    {
        m_saveDownloadToFile = false;
        ok = true;
    }
    return ok;
}


// In COSMLoader.cpp

QByteArray COSMLoader::executeQuerySync(const QString& query, int timeoutMs, QString& errorString)
{
    //Prepare call and post it 
    QByteArray resultData;
    errorString.clear();

    QUrl url("https://overpass-api.de/api/interpreter");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery postData;
    postData.addQueryItem("data", query);

    QNetworkReply* reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->ignoreSslErrors();

    QElapsedTimer elapsed;
    elapsed.start();

    // Loop until the reply is finished OR the timeout is exceeded.
    while (!reply->isFinished() && elapsed.elapsed() < timeoutMs) 
    {
        // Manually process events to allow network signals to be delivered.
        // We use ExcludeUserInputEvents since this is a console application/background task.
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    // 1. Check for Timeout
    if (!reply->isFinished())
    {
        errorString = QString("Host operation timed out after %1 seconds.").arg(timeoutMs / 1000.0);
        reply->abort(); // Abort the ongoing connection
    }
    // 2. Check for Network Error (if it finished before timeout)
    else if (reply->error() != QNetworkReply::NoError)
    {
        errorString = reply->errorString();
    }
    // 3. Check for HTTP Status Code Error
    else
    {
        QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        int httpStatus = statusCode.toInt();

        if (httpStatus >= 200 && httpStatus < 300)
        {
            // Success! The data should be available now.
            resultData = reply->readAll();

            // Check for zero bytes even on success
            if (resultData.isEmpty() && httpStatus == 200) {
                // This is where you catch the "zero bytes" issue when status is OK
                errorString = "Request finished successfully (HTTP 200), but zero bytes were returned.";
            }
        }
        else
        {
            // Server returned an error page (e.g., 400 Bad Request, 500 Internal Error)
            errorString = QString("Overpass Server Error (HTTP %1): %2")
                .arg(httpStatus)
                .arg(QString::fromUtf8(reply->readAll()));
        }
    }

    reply->deleteLater();

    return resultData;
}

QByteArray  COSMLoader::makeCurlCall(const QString& query, int timeoutMs, QString& errorString)
{
    //Result byte array
    QByteArray resultData;
    std::string stdQuery = std::string(query.toUtf8());

        // init the curl session 
    CURL* curlHandle = curl_easy_init();
    if (!curlHandle)
    {
        errorString = "Failed to initialize curlhandle";
        return resultData;
    }

    //Set default values for memory chunk structure 
    struct MemoryStruct chunk;
    chunk.memory = (CHAR*)malloc(1);  // will be grown as needed by the realloc above 
    chunk.size = 0;    // no data at this point 

    // specify URL to get 
    //QString header = "http://overpass-api.de/api/interpreter?data=";
    //QString sURL = header + query;
    //sURL.replace( " ", "%20");
    //QByteArray urlData = sURL.toUtf8();

    std::string stem = "http://overpass-api.de/api/interpreter?data=";
    std::string url = stem + stdQuery;
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str() );


    // Set timeout (in seconds, convert from ms)
    if (timeoutMs > 0) 
    {
        // CURLOPT_TIMEOUT uses seconds, so convert
        long timeoutSec = qMax(1L, (long)(timeoutMs / 1000));
        curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, timeoutSec);
    }

    // send all data to this function  
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    // we pass our chunk struct to the callback function 
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*)&chunk);
    // some servers do not like requests that are made without a user-agent field, so we provide one 
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // get it! 
    CURLcode res  = curl_easy_perform(curlHandle);

    // check for errors 
    if (res != CURLE_OK)
    {
        errorString = "Call with curl failed: " + QString( curl_easy_strerror(res));
        qCCritical( OSMLoaderManagement ) << "curl_easy_perform() failed:" << curl_easy_strerror(res);
        qCCritical(OSMLoaderManagement) << url;
    }
    else
    {
        resultData = QByteArray( chunk.memory, chunk.size);
    }

    // cleanup curl stuff 
    curl_easy_cleanup(curlHandle);
    free(chunk.memory);

        return resultData;
}