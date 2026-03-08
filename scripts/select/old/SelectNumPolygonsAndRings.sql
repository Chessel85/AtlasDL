SELECT
    polygonId,
    MAX( innerPolygonId )
  FROM
    tbl_relationWayLists
  GROUP BY
    polygonId;