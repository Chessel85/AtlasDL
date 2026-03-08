--Return rows to make up a polygon 
SELECT
    wl.innerPolygonId,
    wl.sequenceNumber,
    wl.direction,
    wl.wayId,
    w.firstNodeId,
    w.lastNodeId,
    MbrMinY( w.way1 ),
    MbrMinX( w.way1 ),
    MbrMaxY( w.way1 ),
    MbrMaxX( w.way1 ),
    X( ST_StartPoint( w.way1 ) ),
    Y( ST_StartPoint( w.way1 ) ),
    X( ST_EndPoint( w.way1 ) ),
    Y( ST_EndPoint( w.way1 ) )
  FROM 
    tbl_wayLists  AS wl
  JOIN
    spt_ways AS w ON wl.wayId = w.wayId 
  WHERE
    polygonId = ?
  ORDER BY
    innerPolygonId, sequenceNumber;
