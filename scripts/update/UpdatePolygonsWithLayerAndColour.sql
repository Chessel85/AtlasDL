--Polygon table contains layer id and colour index to improve performance when rendering 
UPDATE spt_polygons
  SET 
    layerId = sub.new_layerId,
    colourIndex = sub.new_colourIndex
  FROM (
    SELECT 
        lp.polygonId AS target_pid,
        lp.layerId AS new_layerId,
        c.colourIndex AS new_colourIndex
    FROM tbl_layerPolygons lp
    -- Join to get the OSM Relation ID for this polygon
    INNER JOIN tbl_polygonOwners po ON lp.polygonId = po.polygonId
    -- Link to the "Sovereign" parent to get the color
    -- This assumes your links table connects child polygons to the country
    LEFT JOIN tbl_links l ON lp.polygonId = l.geomId2 AND l.linkType = 1
    LEFT JOIN tbl_polygonOwners po_parent ON l.geomId1 = po_parent.polygonId
    LEFT JOIN tbl_countries c ON po_parent.elementID = c.RelationID
  ) AS sub
  WHERE spt_polygons.polygonId = sub.target_pid;