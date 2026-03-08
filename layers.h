//Layers.h
#pragma once

#include <QString>
#include <QMap>

class CLayers 
{
    //Constructor
public:
    CLayers();
    ~CLayers();

//Methods
public:
    void clear();
    void addLayer(const QString& layerName, int layerId);
    int getLayerId(const QString& layerName);

    //Attributes
private:
    QMap<QString,int> m_layers;
};
