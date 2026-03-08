--Insert a polygon id into the layer it is in 
INSERT INTO tbl_layerPolygons ( 
    layerId,
    polygonId
) 
VALUES ( ?, ? );
