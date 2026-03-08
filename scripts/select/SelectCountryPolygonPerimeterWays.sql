--Select only ways belonging to a specific country that make up its edge
--This allows a single polygon to be created based on admin level 5 and 6 polygons for the country 
SELECT 
    w.wayId,
    w.firstNodeId,
    w.lastNodeId,
    MbrMinY(w.way1),
    MbrMinX(w.way1),
    MbrMaxY(w.way1),
    MbrMaxX(w.way1),
    X(ST_StartPoint(w.way1)),
    Y(ST_StartPoint(w.way1)),
    X(ST_EndPoint(w.way1)),
    Y(ST_EndPoint(w.way1))
  FROM spt_ways AS w
  WHERE w.wayId IN (
    -- Subquery: Find ways that appear exactly ONCE for this Sovereign ID
    SELECT wl.wayId
    FROM tbl_wayLists wl
    INNER JOIN tbl_layerPolygons lp ON wl.polygonId = lp.polygonId
    INNER JOIN tbl_links l ON lp.polygonId = l.geomId2 AND l.linkType = 1
    INNER JOIN tbl_layerPolygons lp_parent ON l.geomId1 = lp_parent.polygonId AND lp_parent.layerId = 1
    INNER JOIN tbl_polygonOwners po_parent ON lp_parent.polygonId = po_parent.polygonId
    WHERE po_parent.elementID = :countryId   -- Param: The Sovereign Relation ID (e.g., UK)
      AND lp.layerId IN (3, 4)      -- Include Metro (5) and County (6) equivalents
    GROUP BY wl.wayId
    HAVING COUNT(wl.wayId) = 1
  );