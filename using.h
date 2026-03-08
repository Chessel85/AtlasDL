//using.h
#pragma once

#include <QMap>
#include <QList>
#include <map>

class CBaseOSMObject;
class CNode;
class CWay;
class CRelation;
struct PolygonWayEntry;

using NodeMap = std::map<quint64, CNode*>;
using WayMap = QMap<quint64, CWay *>;
using RelationMap = QMap< quint64, CRelation *>;
using NodeList = QList<CNode*>;
using PolygonWayList = QList<PolygonWayEntry>;
