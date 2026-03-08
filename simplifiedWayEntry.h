//simplifiedWayEntry

#pragma once

struct SimplifiedWayEntry 
{
    int polygonId;
    int innerRingId;
    int sequenceNumber;
    quint64 wayId;
    std::string wkt;
};