--Get layerId and layerGroupId given their names
SELECT l.layerId, lg.layerGroupId 
  FROM tbl_layers AS l
  JOIN tbl_layerGroups AS lg ON l.layerGroupId = lg.layerGroupId 
  WHERE 
    l.layerName = :layerName 
    AND   lg.layerGroupId = :groupname;