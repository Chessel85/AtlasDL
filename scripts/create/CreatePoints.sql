--Table to store all the points including metadata 
CREATE TABLE spt_points (
    pointId INTEGER PRIMARY KEY AUTOINCREMENT,
    layerId INTEGER,
    featureClass TEXT,
    name TEXT,
    longitude DOUBLE,
    latitude DOUBLE,
    capital INTEGER,
    minZoom INTEGER,
    wikiDataId TEXT,
    FOREIGN KEY (layerId) REFERENCES tbl_layers( layerId )
);

--Geometry column
SELECT AddGeometryColumn('spt_points', 'pt', 4326, 'POINT', 'XY');

--And spatial index
SELECT CreateSpatialIndex( 'spt_points', 'pt' );

