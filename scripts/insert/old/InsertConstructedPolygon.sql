--Insert polygon constructed from ways
--buildArea can cope with ways in any order and from perimeter and inner rings 
INSERT INTO tmp_polygons ( polygon  )
  SELECT
    ST_ForcePolygonCCW( BuildArea( ST_Collect( collected_way ) ) )
  FROM
  (
    SELECT
        spt_ways.way AS collected_way  
    FROM 
        spt_ways 
    WHERE
        spt_ways.wayId IN (3, 1, 6, 8, 11, 5, 9, 10 )
    );

select astext( polygon ) from tmp_polygons;