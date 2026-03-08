//OSMLoader.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QXmlStreamReader>
#include "BoundingBox.h"
#include "using.h"

class CBaseOSMObject;
class CNode;
class CWay;
class CRelation;

class COSMLoader : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit COSMLoader(QObject* parent = nullptr);
	~COSMLoader();

//Methods
public:
    bool setBoundingBox(const QString& boundingBox);
    bool setXDivisions(const QString& xDivision);
    bool setYDivisions(const QString& yDivision);
    void clearRequiredTags();
    bool appendRequiredTag(const QString& requiredTag);
    void moveToFirstSubBoundingBox();
    bool haveSubBoundingBox();
    bool moveToNextSubBoundingBox();
    bool getRelationIDList(QList<quint64>& relationIDs);
    QString GetSubBoundingBoxAsString();
    bool downloadRelationNWR(quint64 relationID, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster);
    bool downloadWaysNW(NodeMap& rNodesMaster, WayMap& rWaysMaster );
    bool loadRelationsNWRFromFile(const QString& filePath, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster);
    bool setSaveDownloads(const QString& status);

private:
    bool setBoundingBoxFromCountry(const QString& countryName );
    void writeQueryResponseToFile(const QByteArray& data, const QString& filePath);
    bool extractFullNWR(QByteArray& xmlData, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster);
    bool extractFullNW(QByteArray& xmlData, NodeMap& rNodesMaster, WayMap& rWaysMaster);
    bool extractNode(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster);
    bool extractWay(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster, WayMap& rWaysMaster);
    bool extractRelation(QXmlStreamReader& xmlReader, NodeMap& rNodesMaster, WayMap& rWaysMaster, RelationMap& rRelationsMaster);
    bool extractTag(QXmlStreamReader& xmlReader, CBaseOSMObject* pObject);
    bool extractBoundingBox(QXmlStreamReader& xmlReader, double& minlat, double& minlon, double& maxlat, double& maxlon);;
    QByteArray executeQuerySync(const QString& query, int timeoutMs, QString& errorString);
    QByteArray  makeCurlCall(const QString& query, int timeoutMs, QString& errorString);

//Member variables
private:
    CBoundingBox  m_BoundingBox;
    QString m_strBoundingBox;
    int m_xDivisions;
    int m_yDivisions;
    int m_subBBCounter;
    QString m_RequiredTags;    
    bool m_saveDownloadToFile;
    QNetworkAccessManager* m_networkManager;
};