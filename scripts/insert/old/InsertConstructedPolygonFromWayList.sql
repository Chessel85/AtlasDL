-- Insert polygon geometry constructed from ways
-- given just the relation id and polygon id
-- buildArea function can cope with ways in any order and from perimeter and inner rings

INSERT INTO spt_polygons ( elementType, elementId, polygonId, polygon1 )
  VALUES (
    ?,            -- elementType
    ?,            -- elementId (relation id)
    ?,            -- polygonId
    (
        SELECT
            BuildArea( ST_Collect( w.way1 ) ) -- Use w.way1 since we alias spt_ways as w
        FROM
            spt_ways AS w
        LEFT JOIN
            tbl_relationWayLists AS rwl ON w.wayId = rwl.wayId
        WHERE
            rwl.relationId = ? AND rwl.polygonId = ?
    )
);  