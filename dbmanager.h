//DbManager.h
#pragma once

#include <QObject>
#include <QString>
#include <sqlite3.h>


class CDbManager : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CDbManager(QObject* parent = nullptr);
	~CDbManager();

//Methods
public:
    QString getDatabaseName() const;
    bool createOpenDatabase(const QString& databaseName );
    bool readScriptFile(const QString& filename );
    bool executeSqlFile(const QString& scriptFilePath);
    bool executeSqlString(const QString& scriptFilePath);
    sqlite3* getDbHandle() const;
    const char* errorMessage();
    QString getQueryFromScript(const QString& queryFilename);


private:
    bool loadShapefile(const QString& loadDetails);
//Member variables
private:
    QString m_databaseName;
    sqlite3* m_sqlite3db;
};