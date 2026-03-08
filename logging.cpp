//logging.cpp

#include "logging.h"
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <qstandardpaths.h>
#include <QDir>

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) 
{
    // Open the log file
    QString logDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir logDir(logDirectoryPath);

    // Create the directory if it doesn't exist
    if (!logDir.exists()) 
    {
        logDir.mkpath(".");
    }

    // Combine the directory and file name
    //QString logFilePath = logDirectoryPath + "/app.log";
    QString logFilePath = "atlas.log";
    QFile logFile(logFilePath);

    // Get the current timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // Get the message type string
    QString msgTypeString;
    switch (type) {
    case QtDebugMsg:
        msgTypeString = "DEBUG";
        break;
    case QtInfoMsg:
        msgTypeString = "INFO";
        break;
    case QtWarningMsg:
        msgTypeString = "WARNING";
        break;
    case QtCriticalMsg:
        msgTypeString = "CRITICAL";
        break;
    case QtFatalMsg:
        msgTypeString = "FATAL";
        break;
    }

    // Prepare the output string
    QString output = QString("%1: %2 File: %3 Line:%4")
        .arg(msgTypeString)
        .arg(msg)
        .arg(context.file)
        .arg(context.line);
        //.arg(context.function)
        //.arg(timestamp

    // Write to the file
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) 
    {
        QTextStream textStream(&logFile);
        textStream << output << "\n";
        logFile.close();
    }

    // Write to the console
    fprintf(stderr, "%s\n", output.toLocal8Bit().constData());
}