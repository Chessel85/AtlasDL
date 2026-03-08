//multipoly.cpp

#include "multipoly.h"
#include "poly.h"
#include "path.h"
#include <QDebug>
#include "loggingCategories.h"
#include "node.h"
#include "path.h"
#include "paths.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(MultipolyTracker, "Multipoly.tracker")

//Memory management is that CMultipoly is responsible for creating and destroying CPoly objects it contains 
//A CPoly  cannot be shared between two CMultipoly instances 

CMultipoly::CMultipoly()
{
}

CMultipoly::~CMultipoly()
{
    destroyPolygons();
}

void CMultipoly::destroyPolygons()
{
    //Delete each polygon 
    for (CPoly* p : m_polygons )
    {
        delete p;
    }
    m_polygons.clear();
}

iterator CMultipoly::begin()
{
    return m_polygons.begin(); 
}

iterator CMultipoly::end() 
{ 
    return m_polygons.end(); 
}

const_iterator CMultipoly::begin() const
{
    return m_polygons.begin();
}

const_iterator CMultipoly::end()   const 
{ 
    return m_polygons.end(); 
}

int CMultipoly::size() const
{
    return m_polygons.size();
}

bool CMultipoly::isClosed() const
{
    bool isClosed = true;

    //Multipolygon is closed if all constituent polygons are closed 
    for (int i = 0; i < m_polygons.size() && isClosed; i++)
    {
        CPoly* poly = m_polygons.at(i);
        isClosed = poly->isClosed();
    }

    return isClosed;
}

bool CMultipoly::isValid() const
{
    bool isValid = true;

    //Iterate through polygon collection and the multipolygon is valid if all constituent polygons are valid
    for (int i = 0; i < m_polygons.size() && isValid; i++)
    {
        CPoly* poly = m_polygons.at(i);
        isValid = poly->isValid();
    }

    return isValid;
}

bool CMultipoly::assembleFromPaths(CPaths& paths1, CPaths& paths2, bool reversePaths2 )
{
    qCInfo(multipolygonTracker) << "Starting assembly of polygons.";
    //Move all the paths into one collection temporarily 
    CPaths pool;
    QList<CPath*> pathlist = paths1.getPaths();
    for (CPath* path : pathlist )
        pool.addPath(path);

    //Second set of paths reversing if needed 
    pathlist = paths2.getPaths();
    for (CPath* path : pathlist)
    {
        if (reversePaths2)
            path->reverse();
        pool.addPath(path);
    }

    //Go through the pool always pulling off the front item 
    CPath* neoPolygon = nullptr;
    pathlist = pool.getPaths();
    while (pathlist.size() > 0)
    {
        //Seed a new path from the pool as a neo-polygon container 
        CPath* path = pathlist.takeAt(0);
        neoPolygon= new CPath;
        neoPolygon->addPath(path);

//Loop through remaining paths adding them to polygon until it becomes closed 
        int i = 0;
        for (i = 0; i < pathlist.size(); i++)
        {
            //Get tail node every loop as it can change as paths get added 
            quint64 tailNodeId = neoPolygon->getTailNodeId();

            //Get a path from the pool to check 
            CPath* candidatePath = pathlist.at(i);
            quint64 headNodeId = candidatePath->getHeadNodeId();

            //the test
            if (tailNodeId == headNodeId)
            {
                //Attach candidate path to tail of current path  and remove it from the pool
                neoPolygon->addPath(candidatePath);
                pathlist.removeAt(i);

                //if current path is closed add it to the multipolygon and move on 
                if (neoPolygon->isClosed())
                {
                    addPolygon(neoPolygon);
                    delete neoPolygon;
                    neoPolygon= nullptr;
                    break;  //out of loop to start a new neopolygon 
                }

                //Found a path so start the loop again  and go -1 since loop incrementer takes it up to 0 
                i = -1;
            }
        } //Looping around pool 

        //Possible failure condition  if neopolygon was not closed 
        if (neoPolygon )
        {
            qCCritical(multipolygonTracker) << "Failed to close path when assembling polygon.";
            delete neoPolygon;
            pathlist.clear(); //Otherwise destructor will delete contents 
            return false;
        }
    }


    pool.clear();

    return true;
}

void CMultipoly::addPolygon(CPath* closedPath)
{
    CPoly* poly = new CPoly(closedPath);
    m_polygons.append(poly);
}

QString CPoly::getWayIdListAsText()
{
    QString wayIdList;

    //Get way id list from perimeter 
    wayIdList = m_perimeter.getWayIdListAsText();

    //Do the same for each inner ring
    for( CPath* inner : m_innerRings )
    {
        QString innerWayIdList = "," + inner->getWayIdListAsText();
        wayIdList += innerWayIdList;
    }
    return wayIdList;
}

QString CPoly::getWayListAsText()
{
    QString wayList;

    //Get for the perimeter
    wayList = m_perimeter.getWayList( 0 );

    //Do the same for each inner ring
    int ring = 1;
    for (CPath* innerRing : m_innerRings)
    {
        QString innerWayList = innerRing->getWayList(ring++ );
        wayList += innerWayList;
    }

    //Chop off the last comma 
    wayList.chop(1);

    return wayList;
}