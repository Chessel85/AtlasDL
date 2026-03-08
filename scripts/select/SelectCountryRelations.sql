--get all the relations that have polygons in the country layer 
SELECT DISTINCT po.elementId, c.name  
  FROM tbl_polygonOwners  AS po 
    LEFT JOIN tbl_countries AS c ON po.elementID = c.relationId 
    LEFT JOIN tbl_layerPolygons  AS lp ON po.polygonId = lp.polygonId 
  WHERE  lp.layerId = ?; 
