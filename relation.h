//Relation.h
#pragma once

#include "BaseOSMObject.h"
#include "BoundingBox.h"
#include <QString>
#include "using.h"
#include <QList>
#include "wayEntry.h"
#include "PolygonWayEntry.h"

class CNode;
class CWay;

// Struct to hold a node member and its role
struct NodeMember
{
    CNode* node;
    QString role;
};

// Struct to hold a way member and its role
struct WayMember
{
    CWay* way;
    QString role;
};

// Struct to hold a relation member and the role  - only store id value rather than pointer as not loaded 
struct RelationMember
{
    quint64 id;
    QString role;
};


class CRelation : public CBaseOSMObject
{
//Constructor
public:
    CRelation(quint64 id);
    void build(NodeMap& nodesMaster, WayMap& waysMaster, QList<WayEntry>& wayEntries);
    void build(NodeMap& nodesMaster, WayMap& waysMaster, QList<PolygonWayEntry>& relationWayList );
    ~CRelation();

//Methods
public:
    void addNodeMember(CNode* node, const QString& role);
    void addWayMember(CWay* way, const QString& role);
    void addRelationMember(quint64 relationId, const QString& role);
    void setBoundingBox(double south, double west, double north, double east);
    const CBoundingBox& getBoundingBox() const;
    QString getName();
    const CNode* getLabelNode() const;
    WayMember  getWayMemberFromId(quint64 id);
    CWay* getWayFromId(quint64 id);

    const QList<NodeMember>& GetNodeMembers() const;
    const QMap<quint64,WayMember>& GetWayMembers() const;
    const QList<RelationMember>& GetRelationMembers() const;
    void copyWayMembers(QMap<quint64, WayMember>& relationWays );
    void constructWaysForInserting(QString& values ) const;
    void constructTagsForInserting(QString& values) const;

private:
    QList<NodeMember> m_nodeMembers;
    QMap<quint64,WayMember> m_wayMembers;
        QList<RelationMember> m_relationMembers;
    CBoundingBox m_boundingBox;
    QString m_name;
};
