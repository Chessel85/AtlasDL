//BaseOSMObject.h
#pragma once

#include <qglobal.h>
#include <QMap>
#include <QString>
#include <QSet>
#include "Tags.h"

class CBaseOSMObject
{
//Constructors
public:
    explicit CBaseOSMObject(quint64 id);
    virtual ~CBaseOSMObject() = default;

    quint64 getId() const;
    void addTag(const QString& key, const QString& value);
    const CTags& getTags() const;
    void addParent(quint64 parentId );
    const QSet<quint64> getParents() const;

protected:
    quint64 m_id;
    CTags m_tags;
    QSet<quint64> m_parents;
};   