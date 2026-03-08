--Delete a polygon way list ahead of it getting updated 
DELETE FROM tbl_wayLists 
  WHERE polygonId = ?;