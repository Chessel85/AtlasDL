//Relation.cpp

#include "Relation.h"
#include "node.h"
#include "way.h"
#include <QDebug>

CRelation::CRelation(quint64 id)
    : CBaseOSMObject(id)
{
}

CRelation::~CRelation()
{
}

void CRelation::build(NodeMap& nodesMaster, WayMap& waysMaster, QList<PolygonWayEntry>& relationWayList)
{
    //Iterate  through the relation way entries creating nodes and ways for master maps plus way member entries
    for (PolygonWayEntry rwe : relationWayList )
    {
        //Check if nodes already exist and create otherwise
        quint64 headNodeId = rwe.headNodeId;
        if (!nodesMaster.contains(headNodeId))
        {
            CNode* node = new CNode(headNodeId, rwe.headNodeX.c_str(), rwe.headNodeY.c_str()  );
            nodesMaster.emplace(headNodeId, node);
        }
        quint64 tailNodeId = rwe.tailNodeId;
        if (!nodesMaster.contains(tailNodeId))
        {
            CNode* node = new CNode(tailNodeId, rwe.tailNodeX.c_str(), rwe.tailNodeY.c_str()  );
            nodesMaster.emplace(tailNodeId, node);
        }

        //Similarly for the way
        quint64 wayId = rwe.wayId;
        if (!waysMaster.contains(wayId))
        {
            CWay* way = new CWay(wayId);
            waysMaster.insert(wayId, way);

            //Add the nodes and tell the nodes who their parent way is
            CNode* headNode = nodesMaster[rwe.headNodeId];
            CNode* tailNode = nodesMaster[ rwe.tailNodeId];
            headNode->addParent(wayId);
            tailNode->addParent(wayId);

            //Add nodes to way depending on direction
            if (rwe.direction == forward)
            {
                way->addNode(headNode);
                way->addNode(tailNode);
            }
            else
            {
                way->addNode(tailNode);
                way->addNode(headNode);
            }

            //Set bounding box for way and grow relation bounding box 
            way->setBoundingBox(rwe.south, rwe.west, rwe.north, rwe.east);
            m_boundingBox.Grow(way->getBoundingBox());
        }
        //Get the way which either already existed or has just been created 
        CWay* way = nullptr;
        way = waysMaster.value(wayId);
        Q_ASSERT( way );

        //Give the way a parent
        way->addParent(getId());

        //Finally add the way member
        addWayMember(way, "outer");
    }
}

void CRelation::build(NodeMap& nodesMaster, WayMap& waysMaster, QList<WayEntry>& wayEntries)
{
    //Iterate  through the way entries to create relation way entries and then call other version of this method 
    QList<PolygonWayEntry> relationWayList;
    int sequenceNumber = 1;
    for (WayEntry we : wayEntries)
    {
        PolygonWayEntry rwe;
        rwe.polygonId = 1;
        rwe.innerRingId = 0;
        rwe.direction = forward;
        rwe.sequenceNumber = sequenceNumber++;
        rwe.wayId = we.wayId;

        //Node details 
        rwe.headNodeId = we.firstNodeId;
        rwe.tailNodeId = we.lastNodeId;
        rwe.headNodeX = we.startPointX;
        rwe.headNodeY = we.startPointY;
        rwe.tailNodeX = we.endPointX;
        rwe.tailNodeY = we.endPointY;

        //Bounding box 
        rwe.south = we.south;
        rwe.west = we.west;
        rwe.north = we.north;
        rwe.east = we.east;

        relationWayList.append( rwe);
    }

    build(nodesMaster, waysMaster, relationWayList);
}

void CRelation::addNodeMember(CNode* node, const QString& role)
{
    if (node)
    {
        NodeMember member;
        member.node = node;
        member.role = role;
        m_nodeMembers.append(member);
    }
}

void CRelation::addWayMember(CWay* way, const QString& role)
{
    if (way)
    {
        WayMember member;
        member.way = way;
        member.role = role;
        m_wayMembers.insert(way->getId(), member);

        //Grow bounding box
        m_boundingBox.Grow(way->getBoundingBox());
    }
}

void CRelation::addRelationMember(quint64 relationId, const QString& role)
{
    RelationMember member;
    member.id = relationId;
    member.role = role;
    m_relationMembers.append(member);
}

void CRelation::setBoundingBox(double south, double west, double north, double east)
{
    m_boundingBox.setBoundingBox(south, west, north, east);
}

const CBoundingBox& CRelation::getBoundingBox() const
{
    return m_boundingBox;
}

const QList<NodeMember>& CRelation::GetNodeMembers() const
{
    return m_nodeMembers;
}

const QMap<quint64,WayMember>& CRelation::GetWayMembers() const
{
    return m_wayMembers;
}

const QList<RelationMember>& CRelation::GetRelationMembers() const
{
    return m_relationMembers;
}

void CRelation::copyWayMembers(QMap<quint64, WayMember>& relationWays )
{
    for( WayMember mw : m_wayMembers )
    {
        relationWays.insert(mw.way->getId(), mw);
    }
}

void CRelation::constructWaysForInserting(QString& values) const
{
    //Iterate through the way members to add way id, start and end node id and the wkt for the way geometry 
    values = "";
    for (WayMember wm : m_wayMembers)
    {
        QString v = "(" + QString::number( wm.way->getId()  )+ ",";
        CNode* firstNode = wm.way->getFirstNode();
        v += QString::number( firstNode->getId() ) + ",";
        CNode* lastNode = wm.way->getLastNode();
        v += QString::number(lastNode->getId()) + ",";
        wm.way->constructNodesWkt(v);
        v += "),";
        values += v;
    }

    //Exchange the last comma for a close semicolon
    values.replace(values.size() - 1, 1, ';');
}

void CRelation::constructTagsForInserting(QString& values) const
{
    values = "";
    for (auto const& [k, v] : m_tags.asKeyValueRange()) 
    {
        QString value = "(3," + QString::number( m_id) + ",'" + k + "','" + v + "'),";
        values += value;
        }

    //Exchange the last comma for a semicolon 
    values.replace(values.size() - 1, 1, ';');
}


QString CRelation::getName() 
{
    //Get name from tags if not already set
    if (m_name == "")
{
    m_name = m_tags.value("name:en", "");
    if (m_name == "")
        m_name = m_tags.value( "name", "" );
}
return m_name;
}

const CNode* CRelation::getLabelNode() const
{
    //Go through node members to find one with role of label
    CNode* node = nullptr;

    for (NodeMember nm : m_nodeMembers)
    {
        if (nm.role == "label")
        {
            node = nm.node;
            break;
        }
    }

    return node;
}

WayMember CRelation::getWayMemberFromId(quint64 id)
{
    WayMember wayMember;
    if (m_wayMembers.contains(id))
    {
        wayMember = m_wayMembers.value(id);
    }
    else
    {
        wayMember.role = "";
        wayMember.way = nullptr;
    }
    return wayMember;
}

CWay* CRelation::getWayFromId(quint64 id)
{
    CWay* way = nullptr;
    if (m_wayMembers.contains(id))
    {
        WayMember wm = m_wayMembers.value(id);
        way = wm.way;
    }
    return way;
}
