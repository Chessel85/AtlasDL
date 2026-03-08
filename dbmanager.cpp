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
    //Set database name
    m_databaseName = "data/" + databaseName;

    //Check database name is not empty 
    if (m_databaseName == "")
    {
        qCCritical(DbManagerManagement) << "No database name so unable to create it.";
        return false;
    }

    // Find out if database already exists needed later on 
    bool exists = QFile::exists(m_databaseName);

    //Create the sqlite3 instance 
    int rc = sqlite3_open(m_databaseName.toUtf8().constData(), &m_sqlite3db);
    if( rc != SQLITE_OK) 
    {
        qCCritical( DbManagerManagement) << "Failed to create/open database:" << sqlite3_errmsg(m_sqlite3db);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }

    //Enable foreign key cascades
    rc = sqlite3_exec(m_sqlite3db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        qCCritical(DbManagerManagement) << "Failed to enable Foreign Keys:" << sqlite3_errmsg(m_sqlite3db);
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

    //Load extension 
    //rc = sqlite3_load_extension(m_sqlite3db, "mod_spatialite", "sqlite3_spatialite_init",0);
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
    bool res = createApplicationTables("../scripts/create");
    if (!res )
    {
        qCCritical(DbManagerManagement) <<  "Failed to create application tables:" << zErrMsg;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_sqlite3db);
        m_sqlite3db = nullptr;
        return false;
    }
    sqlite3_free(zErrMsg);

    qCInfo(DbManagerManagement) <<  "Successfully created spatialite database and tables.";
    return true;
}

bool CDbManager::createApplicationTables(const QString& scriptsPath) 
{
    if (!m_sqlite3db) 
    {
        qCCritical(DbManagerManagement) << "Database is not open. Cannot execute scripts.";
        return false;
    }
    QDir scriptsDir(QCoreApplication::applicationDirPath() + QDir::separator() + scriptsPath);
    if (!scriptsDir.exists()) 
    {
        qCCritical( DbManagerManagement) << "Script directory does not exist:" << scriptsDir.path();
        return false;
    }

    QStringList filters;
    filters << "*.sql";
    QFileInfoList fileList = scriptsDir.entryInfoList(filters, QDir::Files);

    if (fileList.isEmpty()) 
    {
        qCCritical( DbManagerManagement) << "No SQL files found in the directory:" << scriptsDir.path();
        return false;
    }

    qCInfo(DbManagerManagement) << "Creating application tables with" << fileList.count() << "SQL scripts from" << scriptsDir.path();

    bool success = true;
    for (const QFileInfo& fileInfo : fileList) 
    {
            QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
        {
            qCCritical(DbManagerManagement) << "Could not open SQL file:" << fileInfo.fileName();
            success = false;
            continue;
        }

        QTextStream in(&file);
        QString sqlStatement = in.readAll();
        file.close();

        char* zErrMsg = nullptr;
        int rc = sqlite3_exec(m_sqlite3db, sqlStatement.toUtf8().constData(), nullptr, nullptr, &zErrMsg);
        if (rc != SQLITE_OK) 
        {
            qCWarning( DbManagerManagement) << "Failed to execute SQL from" << fileInfo.fileName() << ":" << zErrMsg;
            sqlite3_free(zErrMsg);
            success = false;
        }
    }

    return success;
}

bool CDbManager::executeSqlFile(const QString& scriptFilePath)
{
    // 1. Check Database State
    if (!m_sqlite3db)
    {
        qCCritical(DbManagerManagement) << "Database is not open. Cannot execute script.";
        return false;
    }

    // 2. Construct Absolute File Path
    // Use QDir::cleanPath to handle separators and simplify the path.
    QString absolutePath = QDir::cleanPath(
        QCoreApplication::applicationDirPath() + QDir::separator() + "../" + scriptFilePath
    );

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

    // 5. Execute SQL using sqlite3_exec
    char* zErrMsg = nullptr;
    // Use .constData() on the UTF-8 converted QString for sqlite3_exec
    int rc = sqlite3_exec(m_sqlite3db, sqlStatement.toUtf8().constData(), nullptr, nullptr, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        qCWarning(DbManagerManagement) << "Failed to execute SQL from" << absolutePath << ":" << zErrMsg;
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