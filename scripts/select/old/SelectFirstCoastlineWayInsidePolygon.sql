--Get the coastline way which has a start node matching given values and is contained within the polygon 
select 
    c.wayId,
    c.firstNodeId 
  from 
    spt_ways as c , 
    spt_polygons  as p 
  where 
    ( c.firstNodeId = ? OR c.firstNodeId = ? )
  And p.elementId = ? AND p.elementType = 3 AND p.polygonId = ? 
  AND c.wayId IN ( SELECT wayId FROM tbl_layerWays )
  AND ST_Contains( p.polygon1, c.way );
