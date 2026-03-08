//paths.h
#pragma once

#include <QHash>
#include "path.h"
#include "BoundingBox.h"

class CMultiPolygon;

class CPaths 
{
//Constructor
public:
    explicit CPaths();
    ~CPaths();

public:
    bool buildFromMultiPolygon(CMultiPolygon& linestrings);
    QList<CPath*> getPaths();
    int getIntersectionType(quint64 anchorHeadNodeId);
    void addPath(CPath* path);
    void clear();
    
private:
    void destroyPaths();

//Member variables
private:
    QList<CPath*> m_paths;
};