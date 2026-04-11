--Table to store all the polygons including metadata 
CREATE TABLE spt_polygons (
    polygonId INTEGER PRIMARY KEY AUTOINCREMENT,
    layerId INTEGER,
    featureClass TEXT,
    name TEXT,
    area DOUBLE,
    minZoomLevel DOUBLE DEFAULT NULL,
    mapColour INTEGER,
    subRegion TEXT,
    minX DOUBLE DEFAULT NULL,
    minY DOUBLE DEFAULT NULL,
    maxX DOUBLE DEFAULT NULL,
    maxY DOUBLE DEFAULT NULL,
    labelX DOUBLE,
    labelY DOUBLE,
    minLabelZoomLevel DOUBLE DEFAULT NULL,
    wikiDataId TEXT,
    FOREIGN KEY (layerId) REFERENCES tbl_layers( layerId )
);

--Index for layerId
CREATE INDEX idx_spt_layer ON spt_polygons(layerId);

--Geometry column
SELECT AddGeometryColumn('spt_polygons', 'polygon', 4326, 'MULTIPOLYGON', 'XY');

--And spatial index
SELECT CreateSpatialIndex( 'spt_polygons', 'polygon' );

