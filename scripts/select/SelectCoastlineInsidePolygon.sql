--Get all coastline ways inside the given polygon geometry 
select 
    c.wayId,
    c.firstNodeId,
    c.lastNodeId,
    MbrMinY( c.way1 ),
    MbrMinX( c.way1 ),
    MbrMaxY( c.way1 ),
    MbrMaxX( c.way1 ),
    X( ST_StartPoint( c.way1 ) ),
    Y( ST_StartPoint( c.way1 ) ),
    X( ST_EndPoint( c.way1 ) ),
    Y( ST_EndPoint( c.way1 ) )
  from 
    spt_ways as c , 
    spt_polygons  as p  
  where 
  p.polygonId = ? 
  AND c.ROWID IN (
        SELECT ROWID
        FROM SpatialIndex
        WHERE
            f_table_name = 'spt_ways'
            AND f_geometry_column = 'way1'
            AND search_frame = p.polygon1 
    )
  AND c.wayId IN ( SELECT wayId FROM tbl_layerWays  WHERE layerId = ? )
  AND ST_Contains(  p.polygon1, c.way1 );
