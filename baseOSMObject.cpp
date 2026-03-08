//BaseOSMObject.cpp

#include "BaseOSMObject.h"

CBaseOSMObject::CBaseOSMObject(quint64 id)
    : m_id(id)
{
}

quint64 CBaseOSMObject::getId() const
{
    return m_id;
}

void CBaseOSMObject::addTag(const QString& key, const QString& value)
{
    m_tags.insert(key, value );
}

const CTags& CBaseOSMObject::getTags() const
{
    return m_tags;
}

void CBaseOSMObject::addParent(quint64 parentId )
{
    //insert automatically checks for duplicates 
    m_parents.insert(parentId );
}

const QSet<quint64> CBaseOSMObject::getParents() const
{
    return m_parents;
}
