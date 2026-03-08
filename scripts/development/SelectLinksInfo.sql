--get info from tbl)links
--default is just for polygon 65 (possibly UK bloated polygon)  and to return 10 entries 
SELECT 
    t1.value,
    po1.elementId,
    l.layerId1,
    t2.value,
    po2.elementId,
    l.layerId2,
    l.geomId1,
    l.geomId2
  FROM tbl_links l
  JOIN tbl_polygonOwners po1 ON l.geomId1 = po1.polygonId
  JOIN tbl_tags t1 ON po1.elementId = t1.elementId 
                AND po1.elementType = t1.elementType 
                AND t1.key = 'name'
  JOIN tbl_polygonOwners po2 ON l.geomId2 = po2.polygonId
  JOIN tbl_tags t2 ON po2.elementId = t2.elementId 
                AND po2.elementType = t2.elementType 
                AND t2.key = 'name'
WHERE l.linkType = 1 -- 'Contains'
  AND l.geomId1 = 65 
  ORDER BY t1.value ASC, t2.value ASC 
  limit 10;
