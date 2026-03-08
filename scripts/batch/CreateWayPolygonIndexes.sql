--Recreate the indexes after simplifying 
SELECT CreateSpatialIndex('spt_ways', 'way1');
SELECT CreateSpatialIndex('spt_ways', 'way2');
SELECT CreateSpatialIndex('spt_ways', 'way3');
SELECT CreateSpatialIndex('spt_ways', 'way4');
SELECT CreateSpatialIndex('spt_polygons', 'polygon1');
SELECT CreateSpatialIndex('spt_polygons', 'polygon2');
SELECT CreateSpatialIndex('spt_polygons', 'polygon3');
SELECT CreateSpatialIndex('spt_polygons', 'polygon4');
CREATE INDEX idx_spt_polygons_displaymin ON spt_polygons (displayMin);
