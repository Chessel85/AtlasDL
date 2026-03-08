//BoundingBox.cpp 
#include "BoundingBox.h"
#include <iostream>
#include <sstream>

CBoundingBox::CBoundingBox()
{
    m_south = 0;
    m_west = 0;
    m_north  = 0;
    m_east = 0;
    m_isNull = true;
}

CBoundingBox::CBoundingBox(const CBoundingBox& bb)
{
    setBoundingBox(bb.m_south, bb.m_west, bb.m_north, bb.m_east);
}


CBoundingBox::~CBoundingBox()
{
}

void CBoundingBox::setBoundingBox(double s, double w, double n, double e)
{
    m_south = s;
    m_west = w;
    m_north = n;
    m_east = e;
    m_isNull = false;
}

bool CBoundingBox::setBoundingBoxFromString(const QString& boundingBox )
{
    //Check input is not empty
    if (boundingBox.isEmpty())
    {
        reset();
        return false;
    }

    // Remove brackets
    if (boundingBox.at(0) != '(' || boundingBox.at(boundingBox.length() - 1) != ')')
    {
        reset();
        return false;
    }

    QString noBrackets = boundingBox.mid(1, boundingBox.length() - 2);

    //Split at commas 
    QStringList parts = noBrackets.split(',');
    if (parts.size() != 4)
    {
        reset();
        return false;
    }

    bool ok = false;
    QString cleanNumber = parts.at(0).trimmed();
    m_south = cleanNumber.toDouble(&ok);
    if (!ok)
    {
        reset();
        return false;
    }

    cleanNumber  = parts.at(1).trimmed();
    m_west = cleanNumber.toDouble(&ok);
    if (!ok)
    {
        reset();
        return false;
    }

    cleanNumber = parts.at(2).trimmed();
    m_north = cleanNumber.toDouble(&ok);
    if (!ok)
    {
        reset();
        return false;
    }

    cleanNumber = parts.at(3);
    m_east = cleanNumber.toDouble(&ok);
    if (!ok)
    {
        reset();
        return false;
    }

    //Getting this far means all is okay 
    m_isNull = false;
    return true;
}

void CBoundingBox::reset()
{
    m_south = 0;
    m_west = 0;
    m_north = 0;
    m_east = 0;
    m_isNull = true;
}

bool CBoundingBox::PointInBox(double x, double y)
{
    bool res = false;

    //Cope with bounding boxes that cross the international date line 
    if (m_west > m_east)
    {
        CBoundingBox IDLBoundingBox;
        IDLBoundingBox.setBoundingBox(m_south, m_west , m_north, m_east+360);
        if (x < 0)
            x += 360;
        if (x <= IDLBoundingBox.m_west && x >= IDLBoundingBox.m_east && y >= IDLBoundingBox.m_south && y <= IDLBoundingBox.m_north)
            res = true;
    }
    else if (x >= m_west && x <= m_east && y >= m_south && y <= m_north)
        res = true;

    return res;
}

void CBoundingBox::Grow(double x, double y)
{
    if (m_isNull)
    {
        m_south = y;
        m_west = x;
        m_north = y;
        m_east = x;
        m_isNull = false;
    }
    else
    {
        if (x < m_west) m_west = x;
        if (x > m_east) m_east = x;
        if (y < m_south) m_south = y;
        if (y > m_north) m_north = y;
    }
}

void CBoundingBox::Grow(const CBoundingBox& rBB)
{
    if (m_isNull)
    {
        m_south = rBB.m_south;
        m_west = rBB.m_west;
        m_north = rBB.m_north;
        m_east = rBB.m_east;
        m_isNull = false;
    }
    else
    {
        if (m_south > rBB.m_south) m_south = rBB.m_south;
        if (m_west > rBB.m_west) m_west = rBB.m_west;
        if (m_north < rBB.m_north) m_north = rBB.m_north;
        if (m_east < rBB.m_east) m_east = rBB.m_east;
    }
}

std::string CBoundingBox::GetBoundingBoxAsString()
{
    std::string s = "";

    if (m_isNull)
    {
        s = "<not defined>";
    }
    else
    {
        s = std::to_string(m_south) + "," + std::to_string(m_west) + "," + std::to_string(m_north) + "," + std::to_string(m_east);
    }

    return s;
}

double CBoundingBox::south()
{
    return m_south;
}

double CBoundingBox::west()
{
    return m_west;
}

double CBoundingBox::north()
{
    return m_north;
}

double CBoundingBox::east()
{
    return m_east;
}

double CBoundingBox::height()
{
    return m_north - m_south;
}

double CBoundingBox::width()
{
    double w = m_west;
    if (m_west > m_east)
        w = m_west - 360;
    return m_east - w;
}

QGeoCoordinate CBoundingBox::getCentre()
{
    double x = m_west + width() / 2;
    double y = m_south + (m_north - m_south) / 2;
    return QGeoCoordinate( y, x );
}

void CBoundingBox::getCentre(double& x, double& y)
{
    x = m_west + width() / 2;
    y = m_south + ( m_north - m_south ) / 2;
}


bool CBoundingBox::Overlap(CBoundingBox& rBB2)
{
    //Overlap means that they intersect or one encloses the other
    if (Contains(rBB2))
        return true;
    if (rBB2.Contains(*this))
        return true;

    //Check for horizontal overlap and if this is true check for vertical overlap 
    if (m_west <= rBB2.m_east && rBB2.m_west <= m_east)
        return m_south <= rBB2.m_north && rBB2.m_south <= m_north;
}

bool CBoundingBox::Contains(CBoundingBox& rBB2)
{
    bool contains = false;

    contains = rBB2.m_south >= m_south && rBB2.m_west >= m_west
        && rBB2.m_north <= m_north && rBB2.m_east <= m_east;

    return contains;
}

bool CBoundingBox::IsNull()
{
    return m_isNull;
}