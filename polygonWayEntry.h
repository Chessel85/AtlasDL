//PolygonWayEntry.h
#pragma once
#include "direction.h"

struct PolygonWayEntry 
{
    int polygonId;
    int innerRingId;
    int sequenceNumber;
    enumDirection direction;
    quint64 wayId;
    quint64 headNodeId;
    quint64 tailNodeId;
    double south;
    double west;
    double north;
    double east;
    std::string headNodeX;
    std::string headNodeY;
    std::string tailNodeX;
    std::string tailNodeY;
};