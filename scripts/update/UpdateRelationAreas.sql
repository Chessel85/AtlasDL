--Update the country relations table summing all constituent polygons 
UPDATE tbl_countries AS r
  SET areaCalculated = (
    SELECT SUM( p.area)
      FROM spt_polygons AS p
    JOIN tbl_polygonOwners AS po
      ON p.polygonId = po.polygonId
    LEFT JOIN tbl_layerPolygons AS lp
      ON lp.polygonId = p.polygonId 
    WHERE po.elementType = 3 -- Element Type 3 is a Relation
      AND po.elementId = r.relationId
      AND lp.layerId != 3 
);