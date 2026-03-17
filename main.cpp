//main.cpp
#include <QCoreApplication>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>      // Defines malloc/free
#include <crtdbg.h>     // Defines the debug heap functions
#include "conductor.h"
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include "loggingCategories.h"
#include "logging.h"

int main(int argc, char *argv[])
{
    //Cause memory leak reporting to happen on exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    //Set a break point when a memory allocation happens at 2327 occurances
    //_CrtSetBreakAlloc(1510681);

    QCoreApplication a(argc, argv);


    // Set the filter rules for logging categories
    QLoggingCategory::setFilterRules("schedule.*.debug=true\ndefault.debug=true");
    // This overrides environment variables and config files
    QLoggingCategory::setFilterRules(
        "polygon.tracker.info=false\n"
        "polygon.tracker.debug=false"
    );

    //Clear the log file by opening it in truncate mode 
    QString logFilePath = "atlas.log";
    QFile logFile(logFilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) 
    {
        QTextStream textStream(&logFile);
        textStream << "--- Application log started at " << QDateTime::currentDateTime().toString() << " ---\n";
        logFile.close();
    }

    // Install the custom message handler
    qInstallMessageHandler(customMessageHandler);

    // Hardcoded schedule file path for this example
    QString scheduleFilename = "natural.txt";

    //Instantiate Conductor to process everything 
    bool result = false;
    CConductor Conductor;
    result = Conductor.init(scheduleFilename);
    if ( result )
    {
        result = Conductor.executeSchedule();
    }

    if (result)
    {
        qInfo() << "Main application finished successfully.";
        QCoreApplication::exit(0);
    }
    else
    {
        qInfo() << "Main application finished with problems.";
        QCoreApplication::exit(1);
    }
}