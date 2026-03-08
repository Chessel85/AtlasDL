SELECT
    c.relationId AS relationId,
    c.name AS country_name,
    COUNT(DISTINCT ipo.polygonId) AS island_count
  FROM
    tbl_layerPolygons AS i
    INNER JOIN tbl_polygonOwners AS po ON i.polygonId = po.polygonId
    INNER JOIN tbl_polygonOwners AS ipo ON i.polygonId = ipo.polygonId AND ipo.elementId != po.elementId
    INNER JOIN tbl_countries AS c ON po.elementId = c.relationId
  WHERE
    i.layerId = 4
  GROUP BY po.elementId, c.name
ORDER BY island_count DESC;