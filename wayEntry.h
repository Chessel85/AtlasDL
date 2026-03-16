//wayEntry.h
#pragma once

struct WayEntry 
{
    quint64 wayId = 0;
    quint64 firstNodeId = 0;
    quint64 lastNodeId = 0;
    double south = 0.0;
    double west = 0.0;
    double north = 0.0;
    double east = 0.0;
    std::string startPointX;
    std::string startPointY;
    std::string endPointX;
    std::string endPointY;
    };