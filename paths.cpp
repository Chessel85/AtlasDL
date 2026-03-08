//paths.cpp

#include "paths.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "directionalWay.h"
#include "polygon.h"
#include "multiPolygon.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(pathsTracker , "paths.tracker")

//Memory management is that CPaths is responsible for creating and destroying CPath objects 
//A CPath cannot be shared between two CPaths instances 

CPaths::CPaths()
{
}

CPaths::~CPaths()
{
    destroyPaths();
}

void CPaths::destroyPaths()
{
    //Delete paths 
    for (CPath* path : m_paths )
    {
        delete path;
    }
    m_paths.clear();
}

bool CPaths::buildFromMultiPolygon(CMultiPolygon& linestrings)
{
    //Loop through each polygon / linestring and create a CPath equivalent 
    linestrings.moveToFirstPolygon();
    CPolygon* polygon = nullptr;
    while( polygon = linestrings.getNextPolygon() ) 
    {
        //Create a CPath and populate it with directional ways from polygon
        CPath* path = new CPath;
        m_paths.append(path);

        polygon->moveToFirstDirectionalWay();
        CDirectionalWay* dw = nullptr;
        while ((dw = polygon->getNextDirectionalWay()))
        {
            path->addDirectionalWay(dw);
        }
    }

    return true;
}

QList<CPath*> CPaths::getPaths()
{
    return m_paths;
}

int CPaths::getIntersectionType(quint64 anchorHeadNodeId)
{
    for( CPath* path : m_paths )
    {
        if (path->getHeadNodeId() == anchorHeadNodeId)
            return 1; //Anchor node is at head of coastline
        else if (path->getTailNodeId() == anchorHeadNodeId)
            return -1; //Anchor node is at tail of coastline 
    }

    return 0;
}

void CPaths::addPath(CPath* path)
{
    m_paths.append(path);
}

void CPaths::clear()
{
    m_paths.clear();
}