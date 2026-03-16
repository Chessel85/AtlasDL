//PolygonWayEntry.h
#pragma once

#include <string>
#include <QtGlobal>
#include "direction.h"

struct PolygonWayEntry
{
    int polygonId = 0;
    int innerRingId = 0;
    int sequenceNumber = 0;
    enumDirection direction = enumDirection::unknown;
    quint64 wayId = 0;
    quint64 headNodeId = 0;
    quint64 tailNodeId = 0;
    double south = 0.0;
    double west = 0.0;
    double north = 0.0;
    double east = 0.0;
    std::string headNodeX;
    std::string headNodeY;
    std::string tailNodeX;
    std::string tailNodeY;
};