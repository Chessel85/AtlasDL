--Drop the indexes  for simplifying 
SELECT DisableSpatialIndex( 'spt_ways', 'way1' );
SELECT DisableSpatialIndex( 'spt_ways', 'way2' );
SELECT DisableSpatialIndex( 'spt_ways', 'way3' );
SELECT DisableSpatialIndex( 'spt_ways', 'way4' );
SELECT DisableSpatialIndex( 'spt_polygons', 'polygon1' );
SELECT DisableSpatialIndex( 'spt_polygons', 'polygon2' );
SELECT DisableSpatialIndex( 'spt_polygons', 'polygon3' );
SELECT DisableSpatialIndex( 'spt_polygons', 'polygon4' );
DROP INDEX idx_spt_polygons_displaymin;

