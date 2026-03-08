//Way.h
#pragma once

#include "baseOSMObject.h"
#include "using.h"
#include "BoundingBox.h"

class CNode;

class CWay : public CBaseOSMObject
{
    //Constructor
public:
    CWay(quint64 id );
    ~CWay();

//Methods
public:
    void addNode( CNode* node);
    CNode*      getFirstNode();
    CNode*      getLastNode();
        void setBoundingBox(double south, double west, double  north, double east);
        CBoundingBox& getBoundingBox();
        NodeList& getNodeList();
        void constructNodesWkt(QString& nodesWkt);
        void dumpInfo();


    //Attribute
private:
    NodeList m_nodes;
    CBoundingBox m_boundingBox;
};
