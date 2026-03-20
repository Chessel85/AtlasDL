cmake_minimum_required(VERSION 3.16)

# 1. STOP CMake from automatically adding UNICODE/UTF-8 flags globally
set(CMAKE_MSVC_UNICODE_DEF OFF)

# 2. Replicate the .pro "NotSet" CharacterSet globally
set(CMAKE_VS_GLOBALS
    "CharacterSet=NotSet"
    "PreferredToolArchitecture=x64"
)

project(AtlasDL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies Path Definitions ---
set(SQLITE3_ROOT "" CACHE PATH "Path to SQLite3")
set(OSGEO4W_ROOT "" CACHE PATH "Path to OSGeo4W")
set(CURL_ROOT    "" CACHE PATH "Path to CURL")
set(GEODESK_ROOT "" CACHE PATH "Path to GeoDesk")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Force the MOC to skip Unicode settings to match our project
list(APPEND QT_MOC_OPTIONS "/UUNICODE" "/U_UNICODE")

find_package(Qt6 COMPONENTS Core Gui Sql Positioning Network REQUIRED)

# Source and header Files ---
set(PROJECT_SOURCES
    main.cpp
    logging.cpp
    conductor.cpp
    processor.cpp
    spatial.cpp
    dbmanager.cpp
    schedule.cpp
    layers.cpp
    OSMLoader.cpp
    geodeskReader.cpp
    baseOSMObject.cpp
    relation.cpp
    way.cpp
    node.cpp
    feature.cpp
    directionalWay.cpp
    path.cpp
    poly.cpp
    multipoly.cpp
    polygon.cpp
    multipolygon.cpp
    paths.cpp
    geoToLambert.cpp
    BoundingBox.cpp
    # --- Headers ---
    logging.h
    loggingCategories.h
    conductor.h
    processor.h
    simplifiedWayEntry.h
    spatial.h
    dbmanager.h
    schedule.h
    layers.h
    OSMLoader.h
    geodeskReader.h
    baseOSMObject.h
    relation.h
    way.h
    node.h
    feature.h
    directionalWay.h
    path.h
    multipoly.h
    poly.h
    polygon.h
    multipolygon.h
    paths.h
    direction.h
    polygonWayEntry.h
    wayEntry.h
    geoToLambert.h
    tags.h
    using.h
    BoundingBox.h
)

# Create the Executable
qt_add_executable(AtlasDL ${PROJECT_SOURCES})

if(MSVC)
    set_target_properties(AtlasDL PROPERTIES 
        VS_GLOBAL_CharacterSet ""
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )

    # 1. Provide C++ code with direct paths to the MASTER folders (No more copying!)
# Provide C++ code with direct paths to the MASTER folders
    # We use escaped quotes so the compiler sees a standard string literal
    target_compile_definitions(AtlasDL PRIVATE 
        _CONSOLE WIN32 WIN64 _MBCS 
        _CRT_NON_CONFORMING_SWPRINTFS 
        _CRT_SECURE_NO_WARNINGS
        _ENABLE_EXTENDED_ALIGNED_STORAGE
        SCRIPTS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/scripts/"
        SHAPEFILES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/shapefiles/"
        SCHEDULES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/schedules/"
    )

    target_compile_options(AtlasDL PRIVATE /UUNICODE /U_UNICODE)

    # Automatically pick the correct GeoDesk lib based on VS Configuration
    target_link_libraries(AtlasDL PRIVATE
        $<$<CONFIG:Debug>:${GEODESK_ROOT}/build/Debug/geodesk.lib>
        $<$<CONFIG:Release>:${GEODESK_ROOT}/build/Release/geodesk.lib>
    )

# Sets AtlasDL as the default "Startup Project" when you open the .sln
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT AtlasDL)
endif()

# --- Include Directories ---
target_include_directories(AtlasDL PRIVATE
    "${SQLITE3_ROOT}/src"
    "${OSGEO4W_ROOT}/include"
    "${CURL_ROOT}/include/curl"
    "${GEODESK_ROOT}/include"
)

# --- Link Libraries ---
target_link_libraries(AtlasDL PRIVATE
    Qt6::Core Qt6::Gui Qt6::Positioning Qt6::Network
    "${SQLITE3_ROOT}/lib/sqlite3.lib"
    "${OSGEO4W_ROOT}/lib/proj.lib"
    "${CURL_ROOT}/libfile/libcurl-x64.lib"
)

# --- Deployment & Asset Handling ---
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET AtlasDL POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime "$<TARGET_FILE:AtlasDL>"
            COMMENT "Gathering Qt dependencies..."
        )
    endif()

    # Ensure 'data' folder exists for logs/output
    add_custom_command(TARGET AtlasDL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AtlasDL>/data"
    )

    # Copy Read-Only assets (NOT scripts or schedules)
    set(ASSET_FOLDERS "documents" "python") 
    foreach(FOLDER ${ASSET_FOLDERS})
        add_custom_command(TARGET AtlasDL POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}"
                "$<TARGET_FILE_DIR:AtlasDL>/${FOLDER}"
        )
    endforeach()
endif()