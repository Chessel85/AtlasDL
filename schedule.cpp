//schedule.cpp

#include "schedule.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "loggingCategories.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(scheduleManagement, "schedule.management")

//List of valid instructions 
const QStringList CSchedule::m_validKeys = 
{
    "database",
    "read",
    "getGeodeskRelations",
    "getGeodeskWays",
    "removeTerritorialWaters",
    "DouglasPeukerTolerance",
    "column",
    "simplifyWaysAndPolygons",
    "zoomBand",
    "areaThreshold",
    "updatePolygonVisibility",
    "end",
    "boundingBox",
    "xDivisions",
    "yDivisions",
    "layerName",
    "layerGeometry",
    "createLayer",
    "saveDownload",
    "requiredTag",
    "excludeByTag",
    "geodeskSource",
    "geodeskFilter",
    "geodeskAreaFilter",
    "preprocessLinks-contains",
    "preprocessLinks-borders",
    "identifyBorders",
    "deriveCountryPolygons",
    "setColours",
    "updatePolygonsWithLayersAndColours",
    "islandsFilter",
    "file"
};

CSchedule::CSchedule( QObject* parent )
    : QObject( parent) 
{
}

CSchedule::~CSchedule()
{
}

bool CSchedule::readInstructionFile(const QString filename)
{
    QString pathAndFilename = SCHEDULES_PATH + filename;
    QFile file(pathAndFilename );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        qCCritical(scheduleManagement ) << "Could not open schedule file:" << pathAndFilename;
        return false;
    }

    QTextStream in(&file);
    QStringList invalidKeys;
    while (!in.atEnd()) 
    {
        QString line = in.readLine().trimmed();
        if( line.isEmpty() || line.startsWith( "//") ) 
            continue;

        //Read the line and check if a valid key 
        QPair<QString,QString> pair = interpretInstruction( line );
        if (!validateKey(pair.first))
        {
            invalidKeys.append(pair.first);
                continue;
        }
        //All good so store the key value pair 
        m_instructions.append(pair);
    }

    file.close();

    //Report failed keys
    if (!invalidKeys.isEmpty())
    {
        qCCritical(scheduleManagement) << "Invalid instructions present in schedule " << filename << "of" << invalidKeys;
        return false;
    }


    if (m_instructions.count() == 0)
    {
        qCCritical(scheduleManagement) << "No instructions loaded from schedule " << filename;
        return false;
    }

    qCDebug( scheduleManagement) << "Successfully read schedule from:" << filename << " containing" << m_instructions.count() << " instructions.";
    
    //Set up the iterator now that instruction list is populated
    m_iterator = m_instructions.begin(); 
    
    return true;
}

bool CSchedule::getNextInstruction( QString& key, QString& value )
{
    QPair<QString, QString> pair;

    if (m_iterator != m_instructions.end()) 
    {
        pair = *m_iterator;
        key = pair.first;
        value = pair.second;
        m_iterator++;
        return true;
    }
    return false;
}

QPair<QString, QString> CSchedule::interpretInstruction(QString& fullInstruction)
{
    //Split the fullInstruction at the first equal sign and set this to the key/value pair
    QPair<QString, QString> pair;
    int firstEqualsIndex = fullInstruction.indexOf( '=' );

    // If an equals sign is found split the string
    if ( firstEqualsIndex != -1 )
    {
        QString key = fullInstruction.left(firstEqualsIndex).trimmed();
        QString value = fullInstruction.mid(firstEqualsIndex + 1).trimmed();
        pair = qMakePair(key, value);
    }
    else
    {
        qCWarning(scheduleManagement) << "Invalid instruction format.";
    }

    return pair;
}

bool CSchedule::validateKey(const QString& key)
{
    //Confirm if the passed in key is on the list of valid keys 
    if( m_validKeys.contains(key, Qt::CaseInsensitive) ) 
    {
        return true;
    }
    return false;
}