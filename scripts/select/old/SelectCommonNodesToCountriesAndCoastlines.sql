--Retrieves nodes that are common to both ways in a country relation polygon and coastline ways
SELECT DISTINCT r.nodeId
  FROM (
    -- Relation polygon way first nodes
    SELECT firstNodeId AS nodeId
    FROM spt_ways
    WHERE wayId IN ( SELECT wayId FROM tbl_wayLists WHERE polygonId = ? )
    UNION
    SELECT lastNodeId AS nodeId
    FROM spt_ways
    WHERE wayId IN (SELECT wayId FROM tbl_wayLists WHERE polygonId = ?)
) AS r
INNER JOIN (
    -- Coastline way nodes
    SELECT firstNodeId AS nodeId
    FROM spt_ways
    WHERE wayId IN ( SELECT wayId FROM tbl_layerWays WHERE layerId = ( SELECT layerId FROM tbl_layers WHERE name = 'coastline' )  )
    UNION
    SELECT lastNodeId AS nodeId
    FROM spt_ways
    WHERE wayId IN (SELECT wayId FROM tbl_layerWays WHERE layerId = ( SELECT layerId FROM tbl_layers WHERE name = 'coastline' ) )
  ) AS c ON r.nodeId = c.nodeId;