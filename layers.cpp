//Layers.cpp

#include "Layers.h"
#include <QDebug>

CLayers::CLayers( )
{
}

CLayers::~CLayers()
{
}

void CLayers::clear()
{
    m_layers.clear();
}

void CLayers::addLayer(const QString& layerName, int layerId)
{
    if (!m_layers.contains(layerName))
        m_layers[layerName] = layerId;
}

int CLayers::getLayerId(const QString& layerName)
{
    int layerId = -1;

    if (m_layers.contains(layerName) )
        layerId = m_layers[layerName];

        return layerId;
}
