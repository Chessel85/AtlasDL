--Get the way that has a first node id as given in the parameter
--Used to help construct coastlines 
SELECT
    w.wayId,
    w.lastNodeId
  FROM 
    spt_ways AS w 
  INNER JOIN tbl_layerWays AS lw ON lw.wayId = w.wayId 
    AND lw.layerId = 2
  WHERE
    firstNodeId = ?;