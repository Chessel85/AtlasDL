//Path.h
#pragma once

#include <QHash>
#include "BoundingBox.h"

class CNode;
class CWay;
class CDirectionalWay;
struct PolygonWayEntry;
class CPath 
{
//Constructor
public:
    explicit CPath();
    ~CPath();

    public:
        quint64 getHeadNodeId() const;
        quint64 getTailNodeId() const;
        CNode* getHeadNode();
        CNode* getTailNode();
        bool isClosed() const;
        bool isValid() const;
        void addDirectionalWay(CDirectionalWay* dw);
        void addWayEntry(PolygonWayEntry& pwe, CWay* way);
        void addPath(CPath* path);
        void reverse();
        void getIntersections(CPath* path, QVector<quint64>& anchors );
        CDirectionalWay* getDirectionalWay(quint64 headNodeId);
        CDirectionalWay* getDirectionalWayByTail(quint64 tailNodeId);
        QString getWayIdListAsText() const;
        QString getWayList(int ringNumber) const;

        private:
            void destroyDirectionalWays();

//Member variables
private:
    QHash<quint64, CDirectionalWay*> m_DwByHead; //Keyed on head node id
    QHash<quint64, CDirectionalWay*> m_DwByTail; //Keyed on tail node id
    quint64 m_headNodeId;
    quint64 m_tailNodeId;
    CBoundingBox m_boundingBox;
};