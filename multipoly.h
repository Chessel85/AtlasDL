//Multipoly.h
#pragma once

#include "BoundingBox.h"
#include "using.h"


class CPaths;
class CPath;
class CPoly;

using iterator = QList<CPoly*>::iterator;
using const_iterator = QList<CPoly*>::const_iterator;

class CMultipoly 
{
//Constructor
public:
    explicit CMultipoly();
    ~CMultipoly();

//Iterator
public:
    iterator begin(); 
    iterator end();

    const_iterator begin() const;
    const_iterator end()   const; 

//Methods 
public:
    int size() const;
    bool isClosed() const;
    bool isValid() const;
    bool assembleFromPaths(CPaths& paths1, CPaths& paths2, bool reversePaths2 = false );

private:
    void destroyPolygons();
    void addPolygon(CPath* closedPath);

//Member variables
private:
    QList<CPoly*> m_polygons;
    CBoundingBox m_boundingBox;
};