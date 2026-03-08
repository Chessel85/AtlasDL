QT = core

#old compiler version CONFIG += c++17 cmdline
#Need v20 for geodesk 
CONFIG += c++20 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 1. REMOVE the Unicode defines that Qt/MSVC often adds by default
DEFINES -= UNICODE
DEFINES -= _UNICODE

# 2. ADD the Multibyte define (Crucial for compatibility with older code)
DEFINES += _MBCS

# If using MSVC (Visual Studio Compiler)
msvc {
    # Ensure any residual flags are also handled, though the above should suffice
    # This specifically targets the Visual Studio compiler build settings
    DEFINES += _CRT_NON_CONFORMING_SWPRINTFS
}



SOURCES += \
    main.cpp \
    logging.cpp \
    conductor.cpp \
    processor.cpp \
    spatial.cpp \
    dbmanager.cpp \
    schedule.cpp \
    layers.cpp \
    OSMLoader.cpp \
    geodeskReader.cpp \
    baseOSMObject.cpp \
    relation.cpp \
    way.cpp \
    node.cpp \
    feature.cpp \
    directionalWay.cpp \
    path.cpp \
    poly.cpp \
    multipoly.cpp \
    polygon.cpp \
    multipolygon.cpp \
    paths.cpp \
    geoToLambert.cpp \
    BoundingBox.cpp


HEADERS += \
    logging.h \
    loggingCategories.h \
    conductor.h \
    processor.h \
    spatial.h \
    dbmanager.h \
    schedule.h  \
    layers.h \
    OSMLoader.h \
    geodeskReader.h \
    baseOSMObject.h \
    relation.h \
    way.h \
    node.h \
    feature.h \
    directionalWay.h \
    path.h \
    multipoly.h \
    poly.h \
    polygon.h \
    multipolygon.h \
    paths.h \
    direction.h \
    polygonWayEntry.h \
    wayEntry.h \
    geoToLambert.h \
    tags.h \
    using.h \
    BoundingBox.h



# Additional modules 
# SQL includes support for sqlite3 
# positioning contains geospatial objects
# GUI contains QPolygonF and related 
# Network handles http calls 
QT += core gui
QT += sql
QT += positioning
QT += network

# --- Specify Paths to SQLite3 and SpatiaLite Headers ---
INCLUDEPATH += "C:/Users/chess/OneDrive/Documents/Chris/Programming/sqlite3/src"

# --- Specify Paths to SQLite3 and SpatiaLite Libraries and Link Them ---
LIBS += -L"C:/Users/chess/OneDrive/Documents/Chris/Programming/sqlite3/lib" -lsqlite3


# Path to PROJ include directory
INCLUDEPATH += "C:/OSGeo4W/include"

# Path to PROJ library directory
LIBS += -L"C:/OSGeo4W/lib"

# Link against the PROJ library
LIBS += -lproj


# Path to curl include 
INCLUDEPATH += "C:\Users\chess\OneDrive\Documents\Chris\Programming\CURL\include\curl"

# Path to curl library directory
LIBS += -L"C:\Users\chess\OneDrive\Documents\Chris\Programming\CURL\libfile"

# Link against the curl library
LIBS += -llibcurl-x64

# --- GeoDesk Library Configuration (Using Compiled Paths) ---

# 1. Include Path (where the compiler finds .h files)
# This usually points to the 'include' directory in your source tree.
INCLUDEPATH += "C:\Users\chess\OneDrive\Documents\Chris\Programming\Geodesk\libgeodesk-NumberedNodes\include"

# 2. Linker Settings for Debug and Release

# DEBUG Configuration: Points to the Debug folder and links the debug library
CONFIG(debug, debug|release) {
    LIBS += -L"C:\Users\chess\OneDrive\Documents\Chris\Programming\Geodesk\libgeodesk-NumberedNodes\build\Debug"
    LIBS += -lgeodesk   # Adjust the library name if it's different (e.g., libgeodesk_d)
}

# RELEASE Configuration: Points to the Release folder and links the release library
CONFIG(release, debug|release) {
    LIBS += -L"C:\Users\chess\OneDrive\Documents\Chris\Programming\Geodesk\libgeodesk-NumberedNodes\build\Release"
    LIBS += -lgeodesk    # Adjust the library name if it's different
}





# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
