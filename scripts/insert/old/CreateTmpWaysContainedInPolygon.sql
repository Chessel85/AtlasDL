DROP TABLE IF EXISTS tmp_containedWays;

CREATE TEMP TABLE tmp_containedWays AS
SELECT
    c.wayId,
    c.firstNodeId,
    c.lastNodeId
  FROM
    spt_polygons AS p,
    spt_ways AS c
  WHERE
    p.elementId = 87565
    AND p.elementType = 3
    AND p.polygonId = 2
    AND c.wayId IN (
        SELECT wayId
        FROM tbl_layerWays
        WHERE layerId = (SELECT layerId FROM tbl_layers WHERE name = 'coastline')
    )
    AND p.ROWID IN (
        SELECT ROWID
        FROM SpatialIndex
        WHERE
            f_table_name = 'spt_polygons'
            AND f_geometry_column = 'polygon'
            AND search_frame = c.way
    )
    AND ST_Contains(p.polygon, c.way);