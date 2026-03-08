//DirectionalWay.h
#pragma once
#include "using.h"
#include "direction.h"
#include "PolygonWayEntry.h"

class CNode;
class CWay;

class CDirectionalWay 
{
//Constructor
public:
    CDirectionalWay( CWay* way, enumDirection direction  );
    CDirectionalWay(PolygonWayEntry& pwe, CWay* way );
    CDirectionalWay(CDirectionalWay* dw);

//Methods
public:
    CWay* getWay();
    CNode* getHeadNode();
    CNode* getTailNode();
    quint64 getWayId();
    quint64  getHeadNodeId(); //Respects direction flag 
    quint64 getTailNodeId(); //Respects direction flag
    enumDirection getDirection();
    void reverse();
    void set(CWay* way, enumDirection direction);
    void getNodeList(NodeList& nodes);


//Member variables
private:
    CWay* m_way;
    quint64 m_wayId;
    enumDirection m_direction;
    quint64 m_headNodeId;
    quint64 m_tailNodeId;
};