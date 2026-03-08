--Insert bordering relationships for polygons in the links table 
-- 1. Create a fast temporary list of all candidates
WITH WaySharingPairs AS (
    SELECT DISTINCT
        w1.polygonId AS p1,
        lp1.layerId AS l1,
        w2.polygonId AS p2,
        lp2.layerId AS l2
    FROM tbl_wayLists w1
    INNER JOIN tbl_wayLists w2 ON w1.wayId = w2.wayId
    INNER JOIN tbl_layerPolygons lp1 ON w1.polygonId = lp1.polygonId
    INNER JOIN tbl_layerPolygons lp2 ON w2.polygonId = lp2.polygonId
    WHERE w1.polygonId < w2.polygonId -- This ensures we only get (A,B), not (B,A) and (A,A)
  )
-- 2. Insert into links, filtering out containment pairs in bulk
  INSERT INTO tbl_links (layerId1, geomId1, linkType, layerId2, geomId2)
    SELECT l1, p1, 10, l2, p2
    FROM WaySharingPairs wsp
    LEFT JOIN tbl_links existing 
        ON (existing.geomId1 = wsp.p1 AND existing.geomId2 = wsp.p2 AND existing.linkType = 1)
        OR (existing.geomId1 = wsp.p2 AND existing.geomId2 = wsp.p1 AND existing.linkType = 1)
    WHERE existing.geomId1 IS NULL;