select r.relationID, t.value  
 from tbl_Relations AS r 
  JOIN tbl_Tags AS t ON t.elementID = r.relationID 
 WHERE
  t.key='wikidata'
 AND r.area IS NULL;
