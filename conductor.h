//Conductor.h
#pragma once

#include <QObject>
#include <QString.h>
#include "using.h"
#include "schedule.h"
#include "layers.h"
#include "OSMLoader.h"
#include "geodeskReader.h"
#include "Tags.h"

class CDbManager;
class CSpatial;
class CProcessor;
class CNode;


class CConductor : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CConductor( QObject* parent = nullptr);
	~CConductor();

//Methods
public:
    bool init(const QString& scheduleFilename);
    bool executeSchedule();

private:
    void loadSettings();
    bool insertLayer(const QString& layerName, const QString& layerGeometry );
    void deleteMasters();
    bool DownloadAndprocessRelations();
    bool DownloadAndprocessWays();
    bool ProcessFromFile( const QString& filename );
    //CNode* getNode(quint64 nodeId, double longitude, double latitude, NodeMap& nodesMaster);
    bool assignColourIndex(const QString& sLayerId );
    void clearFilters();
    bool ifDoingCountryLayer();
    bool ifDoingIslandLayer();
    bool simplifyWaysAndPolygons();
    bool identifyBorders(QString& layersInfo);

//Member variables
private:
    CSchedule m_schedule;
    CLayers m_layers;
    CSpatial* m_spatial;
    CDbManager* m_DbManager;
    COSMLoader m_OSMLoader;
    CProcessor* m_processor;
    CGeodeskReader m_geodeskReader;

    QString m_CountryLayerName;
    QString m_islandLayerName;
    QString m_coastlineLayerName;
    QString m_territorialWatersLayerName;
    QString m_newLayerName;
    QString m_newLayerGeometry;

    int m_currentLayerId;
    QString m_currentLayerName;

    //Master lists
    NodeMap  m_nodesMaster;
    WayMap m_waysMaster;
    RelationMap m_relationsMaster;
};