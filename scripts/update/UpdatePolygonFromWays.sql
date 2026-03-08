--Update polygon constructed from ways
--buildArea can cope with ways in any order and from perimeter and inner rings 
UPDATE spt_polygons
  SET polygon1 = (
    SELECT
        ST_ForcePolygonCCW( BuildArea( ST_Collect( collected_way ) ) )
    FROM
        (
            SELECT
                spt_ways.way1 AS collected_way
            FROM
                spt_ways
            WHERE
                spt_ways.wayId IN ??? 
        )
    )
    WHERE 
        polygonId = ?;