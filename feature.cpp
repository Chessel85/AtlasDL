//Feature.cpp

#include "Feature.h"
#include "node.h"
#include "way.h"
#include "relation.h"
#include "directionalWay.h"
#include "polygon.h"
#include <QSet>
#include <QDebug>
#include "loggingCategories.h"

//Set up logging category hierarchy
Q_LOGGING_CATEGORY(featureInformation , "Feature.information")


CFeature::CFeature( CRelation* relation, QObject* parent )
    : m_relation( relation ), QObject( parent) 
{
}

CFeature::~CFeature()
{
}


bool CFeature::isEmpty()
{
    return (m_multipolygon.isEmpty() && m_innerPolygons.isEmpty() && m_lineStrings.isEmpty());
}

bool CFeature::deducePolygons()
{
    //Check the relation is set
    if( !m_relation )
        return false;

    //Work through the ways in the relation
    //For each way  ask the last node for a way with the same relation id
    //Keep going until either no way or join up with start node 

    //First need to extract all the ways into a separate list so we can remove ways once added to a waylist 
    QMap<quint64, WayMember> relationWays;
    m_relation->copyWayMembers(relationWays );

    //Keep track of number of ambiguous nodes encountered 
    int internalAmbiguousNodeCounter = 0;
    int outerAmbiguousNodeCounter = 0;
    int crossRoleCounter = 0;

    //Loop through the ways until all gone
    while (relationWays.size() > 0)
    {
        //Start with the next available way member 
        WayMember firstWayMember = relationWays.first();
        QString role = firstWayMember.role;

        //Get head and tail ways  which by definition for the first in the list is the first way 
        CWay* headWay = firstWayMember.way;
        CWay* tailWay = headWay;

        //Create a new polygon  which gets added to a polygon collection after building it has finished so it then gets deleted in the destructor 
        CPolygon* polygon = new CPolygon(headWay, forward);
        relationWays.remove(headWay->getId() );

        //Do a while loop building the directional way list as long as there are more ways and a loop has not been made
        bool canContinue = true;

        //Need to capture special case of a polygon made of just one way
        if (polygon->isClosed())
            canContinue = false;
        while (canContinue)
        {
            //debug code
            if (polygon->numWays() == 15 )
                int k = 0;
            //Get tail node of polygon to see which other ways this node has as parents 
            CNode* tailNode = polygon->getTailNode();
            QSet<quint64> nodeParents = tailNode->getParents();
            //Find the next way from the node parents 
            if (nodeParents.size() > 2)
            {
                if (role == "inner")
                    internalAmbiguousNodeCounter++;
                else
                    outerAmbiguousNodeCounter++;
            }
            WayMember nextWayMember;
            nextWayMember.way = nullptr;
            for (quint64 nodeParentId : nodeParents)
            {
                //Need to blank the way as otherwise can be carried forward from previous go around the loop 
                nextWayMember.way = nullptr;  

                //Exclude id if same as current way
                if (tailWay->getId() == nodeParentId)
                    continue;
                nextWayMember = relationWays.value(nodeParentId );
                if (nextWayMember.way  ) //this filters out ways from other relations  as would get null if not in current relation  
                {
                    break;
                }
            }

            //If a next way was found stitch it onto the end of the current polygon 
            if (nextWayMember.way )
            {//Role check
                if (nextWayMember.role != role)
                    crossRoleCounter++;
                polygon->growPolygon(nextWayMember.way );
                tailWay = nextWayMember.way;

                //debug code
                if (tailWay->getId() == 494860302 || tailWay->getId() == 494860303) 
                    int k = 0;
                relationWays.remove(nextWayMember.way->getId() );

                //Check if current polygon is a loop
                if( polygon->isClosed() )
                    canContinue = false;
            }
            else
            {
                //There was no next way so a dead end 
                canContinue = false;
            }
        }
        //Have finished constructing polygon or line string  so give it to a multipolygon to own 
        if (polygon->isClosed())
        {
            //Depends on role 
            if (role == "outer")
                m_multipolygon.addPolygon(polygon);
            else if (role == "inner")
                m_innerPolygons.addPolygon(polygon);
            else
            {
                qCWarning(featureInformation) << "Relation" << m_relation->getName() << "(" << m_relation->getId() << ") has an unknown role. '" << role << "'. Deleting polygon to prevent memory leak.";
                delete polygon;
            }
        }
        else //not a closed polygon so add to line string collection 
        {
            m_lineStrings.addPolygon(polygon);
        }
    }

    //Line strings may still concatenate together to form longer line strings 
    int preJoin = m_lineStrings.numPolygons();
    m_lineStrings.joinLineStrings();

    //Output info
    qCInfo(featureInformation) << m_multipolygon.numPolygons() << "polygons deduced." << m_innerPolygons.numPolygons() << "inner rings deduced." << preJoin << "line strings resolved to" << m_lineStrings.numPolygons() << "line strings.";
    if (internalAmbiguousNodeCounter > 0 || outerAmbiguousNodeCounter > 0 || crossRoleCounter > 0)
        qCInfo(featureInformation) << "Encountered" << internalAmbiguousNodeCounter << "internal ambiguous nodes," << outerAmbiguousNodeCounter << "outer ambiguous nodes and" << crossRoleCounter << "mismatching roles on joining ways.";

    if (!m_lineStrings.isEmpty())
    {
        //qDebug() << m_lineStrings.numPolygons() << "line strings exist.";
        //m_lineStrings.dumpInfo2();
    }

    return true;
}

CMultiPolygon& CFeature::getPolygons()
{
    return m_multipolygon;
}

CMultiPolygon& CFeature::getInnerRings()
{
    return m_innerPolygons;
}

CMultiPolygon& CFeature::getLinestrings()
{
    return m_lineStrings;
}

