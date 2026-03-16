//simplifiedWayEntry

#pragma once

struct SimplifiedWayEntry 
{
    int polygonId = 0;
    int innerRingId = 0;
    int sequenceNumber = 0;
    quint64 wayId = 0;
    std::string wkt;
};