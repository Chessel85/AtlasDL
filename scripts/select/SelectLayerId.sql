--Select layer id given group name and layer name
SELECT layerId
  FROM tbl_layers AS l
  JOIN tbl_layerGroups AS lg ON lg.layerGroupId = l.layerGroupId
  WHERE 
    layerGroupName = :groupName AND
    layerName = :layerName;