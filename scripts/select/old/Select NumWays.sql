SELECT
    polygonId,
  innerPolygonId,
    MAX(  sequenceNumber )
  FROM
    tbl_relationWayLists
  GROUP BY
    polygonId, innerPolygonId ;