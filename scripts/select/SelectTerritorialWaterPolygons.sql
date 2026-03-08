--Get territorial water polygon ids and the country relation they belong to
SELECT
    po.polygonId,
    po.elementType,
    po.elementId
  FROM
    tbl_polygonOwners AS po
    LEFT JOIN tbl_layerPolygons AS lp ON lp.polygonId = po.polygonId 
  WHERE
    lp.layerId = ?; --layer id for territorial waters 