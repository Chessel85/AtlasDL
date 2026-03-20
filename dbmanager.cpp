//DbManager.cpp

#include "DbManager.h"
#include <QCoreApplication>
#include <QSet>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "loggingCategories.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(DbManagerManagement, "DbManager.management")

CDbManager::CDbManager( QObject* parent )
    : QObject( parent) 
{
    m_sqlite3db = nullptr;
    m_databaseName = "";
    }

CDbManager::~CDbManager()
{
    // Close the database connection if it is open
    if (m_sqlite3db != nullptr)
    {
        int rc = sqlite3_close(m_sqlite3db);
        if (rc != SQLITE_OK)
        {
            qCWarning(DbManagerManagement) << "Failed to close database.";
        }
        else
        {
            qCInfo(DbManagerManagement) << "Database connection closed successfully.";
        }

        m_sqlite3db = nullptr; 
    }
}

QString CDbManager::getDatabaseName() const
{
    return m_databaseName;
}

bool CDbManager::createOpenDatabase(const QString& databaseName )
{
    //Check database name is not empty 
    if (databaseName == "")
    {
        qCCritical(DbManagerManagement) << "No database name so unable to create it.";
        return false;
    }

    //Add path to database name
    QString appDir = QCoreApplication::applicationDirPath();
    m_databaseName = appDir + "/data/" + databaseName;

    // Find out if database already exists needed later on 
    bool exists = QFile::exists(m_databaseName);

    //Create the sqlite3 instance 
    qCInfo(DbManagerManagement) << "Opening database" << m_databaseName;
    int rc = sqlite3_open(m_databaseName.toUtf8().constData(), &m_sqlite3db);
    if( rc != SQLITE_OK) 
    {
        qCCritical( DbManagerManagement) << "Failed to create/open database" << m_databaseName << ":" << sqlite3_errmsg(m_sqlite3db);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    //Enable foreign key cascades
    rc = sqlite3_exec(m_sqlite3db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        qCCritical(DbManagerManagement) << "Failed to enable Foreign Key cascades:" << sqlite3_errmsg(m_sqlite3db);
    }

    // Make it a Spatialite database by loading the spatialite extension and creating metadata
    // NOTE: The "mod_spatialite" library file must be in a location where sqlite3 can find it
    qCInfo( DbManagerManagement) << "Loading spatialite extension...";

    rc = sqlite3_enable_load_extension(m_sqlite3db, 1);
    if (rc != SQLITE_OK) 
    {
        qCCritical( DbManagerManagement) << "Failed to enable extensions:" << sqlite3_errmsg(m_sqlite3db);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    //Load spatialite extension 

    rc = sqlite3_load_extension(m_sqlite3db, "mod_spatialite", 0, 0);
    if (rc != SQLITE_OK) 
    {
        qCCritical(DbManagerManagement) <<  "Failed to load spatialite extension:" << sqlite3_errmsg(m_sqlite3db);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    //If opening an existing database then complete at this point
    if (exists)
    {
        qCInfo(DbManagerManagement) << "Successfully opened database " << m_databaseName;
        return true;
    }

    // Initialize the spatialite metadata 
    char* zErrMsg = nullptr;
    rc = sqlite3_exec(m_sqlite3db, "SELECT InitSpatialMetaDataFull(true);", nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) 
    {
        qCCritical(DbManagerManagement) <<  "Failed to initialize Spatialite metadata:" << zErrMsg;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    // Add application specific tables
    bool res = readScriptFile( "CreateBaseTables.spatialite" );
    if (!res )
    {
        qCCritical(DbManagerManagement) <<  "Failed to create application tables:" << zErrMsg;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    qCInfo(DbManagerManagement) <<  "Successfully created spatialite database and tables.";
    return true;
}
bool CDbManager::readScriptFile(const QString&  filename )
{
    //Check health of database 
    if (!m_sqlite3db)
    {
        qCCritical(DbManagerManagement) << "Database is not open.";
        return false;
    }

    // Build full path for a script 
    QString absolutePath = SCRIPTS_PATH + filename;

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCCritical(DbManagerManagement) << "Could not open file:" << absolutePath;
        return false;
    }

    QTextStream in(&file);
    QString sqlAccumulator;
    bool success = true;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // Skip comments and empty lines
        if (line.isEmpty() || line.startsWith("--")) 
        {
            continue;
        }

        if (line.startsWith(".read"))
        {
            // 1. Handle recursive script reading
            QString subFileName = line.section(' ', 1).trimmed();
            if (!readScriptFile(subFileName)) 
            {
                success = false;
            }
        }
        else if (line.startsWith(".load"))
        {
            // 2. Handle shapefile loading
            QString shapefilePath = line.section(' ', 1).trimmed();
            if (!loadShapefile(shapefilePath)) 
            {
                success = false;
            }
        }
        else
        {
            // 3. Accumulate standard SQL lines
            sqlAccumulator.append(line + "\n");
        }
    }
    file.close();

    // 4. Execute any accumulated SQL at the end of the file
    if (success && !sqlAccumulator.trimmed().isEmpty())
    {
        if (!executeSqlString(sqlAccumulator)) 
        {
            qCCritical(DbManagerManagement) << "Failed to execute SQL block from:" << filename;
            success = false;
        }
    }

    return success;
}


/*
bool CDbManager::readScriptFile(const QString& filename )
{
    //Check database is okay 
    if (!m_sqlite3db)
    {
        qCCritical(DbManagerManagement) << "Database is not open.";
        return false;
    }

    // Resolve full path 
    QString fullPath = SCRIPTS_PATH + filename;
    QFile masterFile(SCRIPTS_PATH + filename );

    if (!masterFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCCritical(DbManagerManagement) << "Could not open script file:" << fullPath;
        return false;
    }

    QTextStream in(&masterFile);
    bool allSuccess = true;

    while (!in.atEnd() && allSuccess )
    {
        QString line = in.readLine().trimmed();

        // 1. Ignore empty lines or comments starting with "--"
        if (line.isEmpty() || line.startsWith("--"))
        {
            continue;
        }

        // 2. Look for lines starting with ".read"
        if (line.startsWith(".read"))
        {
            // Extract filename: split by space and take the second part
            // Using Section handles cases with multiple spaces effectively
            QString sqlFileName = line.section(' ', 1, 1).trimmed();

            if (sqlFileName.isEmpty())
            {
                qCWarning(DbManagerManagement) << "Found .read command but no filename specified.";
                allSuccess = false;
                continue;
            }

            if (!executeSqlFile(sqlFileName ))
            {
                qCCritical(DbManagerManagement) << "Failed to execute script:" << sqlFileName;
                allSuccess = false;
            }
        }
    }

    masterFile.close();
    return allSuccess;
}*/

bool CDbManager::loadShapefile(const QString& commandArgs)
{
    // Expected args: "shpPath tableName charset srid  geomType"
    QStringList args = commandArgs.split(' ', Qt::SkipEmptyParts);

    if (args.size() < 5)
    {
        qCCritical(DbManagerManagement) << "Insufficient arguments for virtual load. Expected 5:" << commandArgs;
        return false;
    }

    QString basePath = SHAPEFILES_PATH + args.at(0);
    QString tableName = args.at(1);
    QString charset = args.at(2);
    QString srid = args.at(3);
    QString geomType = args.at(4).toUpper(); // e.g., 'POLYGON', 'POINT'

    //Check file with .shp added exists 
    if (!QFile::exists(basePath+ ".shp"))
    {
        qCCritical(DbManagerManagement) << "Shapefile not found at:" << basePath+ ".shp";
        return false;
    }

    // use a unique name for the virtual link to avoid collisions
    QString virtualTab = "vrt_" + tableName;

    // Sequence of SQL commands to perform the import
    QStringList sqlCommands;

    // 1. Create the virtual link to the physical files (.shp, .dbf, .shx)
    sqlCommands << QString("CREATE VIRTUAL TABLE %1 USING VirtualShape('%2', '%3', %4);")
        .arg(virtualTab, basePath, charset, srid);

    // 2. Transfer data into a permanent table
    // Note: VirtualShape always names the geometry column "Geometry"
    sqlCommands << QString("CREATE TABLE %1 AS SELECT * FROM %2;")
        .arg(tableName, virtualTab);

    // 3. Drop the virtual link (does not delete the source files)
    sqlCommands << QString("DROP TABLE %1;").arg(virtualTab);

    // 4. Register the geometry column in spatial_ref_sys / geometry_columns
    // RecoverGeometryColumn(table, column, srid, geom_type, dimension)
    sqlCommands << QString("SELECT RecoverGeometryColumn('%1', 'Geometry', %2, '%3', 'XY');")
        .arg(tableName, srid, geomType);

    qCInfo(DbManagerManagement) << "Importing Shapefile via Virtual Table:" << basePath;

    char* zErrMsg = nullptr;
    for (const QString& sql : sqlCommands)
    {
        int rc = sqlite3_exec(m_sqlite3db, sql.toUtf8().constData(), nullptr, nullptr, &zErrMsg);

        if (rc != SQLITE_OK)
        {
            qCCritical(DbManagerManagement) << "SQL Error during Virtual Import:" << zErrMsg;
            qCCritical(DbManagerManagement) << "Failed Statement:" << sql;

            // Critical: If step 1 or 2 fails, we should still try to drop the virtual table
            sqlite3_exec(m_sqlite3db, QString("DROP TABLE IF EXISTS %1;").arg(virtualTab).toUtf8().constData(), nullptr, nullptr, nullptr);

            sqlite3_free(zErrMsg);
            return false;
        }
    }

    return true;
}

bool CDbManager::executeSqlFile(const QString& scriptFilePath)
{
    // 1. Check Database State
    if (!m_sqlite3db)
    {
        qCCritical(DbManagerManagement) << "Database is not open. Cannot execute script.";
        return false;
    }

    // Build file path 
    // QString absolutePath = QDir::cleanPath( QCoreApplication::applicationDirPath() + QDir::separator() + "../" + scriptFilePath );
    QString absolutePath = SCRIPTS_PATH + scriptFilePath;

    QFile file(absolutePath);

    // 3. Check File Existence and Open
    if (!file.exists())
    {
        qCCritical(DbManagerManagement) << "SQL script file does not exist:" << absolutePath;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCCritical(DbManagerManagement) << "Could not open SQL file:" << absolutePath;
        return false;
    }

    // 4. Read File Content
    QTextStream in(&file);
    QString sqlStatement = in.readAll();
    file.close();

    qCInfo(DbManagerManagement) << "Executing SQL script from:" << absolutePath;

    return executeSqlString(sqlStatement);;
}

bool CDbManager::executeSqlString(const QString& sql)
{
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(m_sqlite3db, sql.toUtf8().constData(), nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        qCWarning(DbManagerManagement) << "SQL Error:" << zErrMsg;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

sqlite3* CDbManager::getDbHandle() const
{
    return m_sqlite3db;
}

const char* CDbManager::errorMessage()
{
    if (!m_sqlite3db)
        return nullptr;

    return sqlite3_errmsg(m_sqlite3db);
}

QString CDbManager::getQueryFromScript(const QString& queryFilename)
{
    QFile file(queryFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Handle error if file cannot be opened.
        qCWarning(spatialManagement) << "Error opening file:" << queryFilename;
        return QString();
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return content;
}