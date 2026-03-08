--Calculate the length of the borders between polygons 
UPDATE tbl_links
SET 
    intersectionSize = (
        SELECT SUM(ST_Length(w.way1, 1)) 
        FROM tbl_wayLists AS wl1
        JOIN tbl_wayLists AS wl2 ON wl1.wayId = wl2.wayId
        JOIN spt_ways AS w ON wl1.wayId = w.wayId
        WHERE wl1.polygonId = tbl_links.geomId1 
          AND wl2.polygonId = tbl_links.geomId2
    ),
    intersectionDimension = 1 -- 1D = Linear/Way intersection
WHERE linkType = 10;