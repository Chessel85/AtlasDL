//wayEntry.h
#pragma once

struct WayEntry 
{
    quint64 wayId;
    quint64 firstNodeId;
    quint64 lastNodeId;
    double south;
    double west;
    double north;
    double east;
    std::string startPointX;
    std::string startPointY;
    std::string endPointX;
    std::string endPointY;
    };