--Work out from relation way table which relations share ways and hence border each other 
--Constraint on border table prevents duplicate from and to relation id
INSERT OR IGNORE INTO tbl_Borders (fromRelationName, toRelationName, fromRelationID, toRelationID, maritime) 
  SELECT
    t3.value AS relationName1,
    t4.value AS relationName2,
    t1.relationId AS relationId1,
    t2.relationId AS relationId2,
    ? AS maritime
  FROM
    tbl_relationWayLists AS t1
  INNER JOIN
    tbl_relationWayLists AS t2 ON t1.wayId = t2.wayId  
   AND t1.relationId != t2.relationId 
  INNER JOIN
    tbl_tags AS t3 ON t1.relationId = t3.elementId AND t3.key = 'name:en'
  INNER JOIN
    tbl_tags AS t4 ON t2.relationId = t4.elementId AND t4.key = 'name:en'
  GROUP BY
    relationId1,
    relationId2;