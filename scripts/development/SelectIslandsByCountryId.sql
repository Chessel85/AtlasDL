SELECT DISTINCT 
    count( it.value  )
  FROM 
    tbl_layerPolygons AS i 
    LEFT JOIN tbl_polygonOwners AS po ON i.polygonId = po.polygonId 
    LEFT JOIN tbl_polygonOwners AS ipo ON ipo.polygonId = po.polygonId AND ipo.elementId != 62149 
    LEFT JOIN tbl_tags AS it ON it.elementId = ipo.elementId 
  WHERE 
    i.layerId = 4
    AND po.elementId = 62149
    AND it.key='name' 
  ORDER BY  it.value ASC  ;