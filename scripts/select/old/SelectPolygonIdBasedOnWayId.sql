--Get the polygon ids which include the given way id
SELECT
    polygonId
  FROM
    tbl_wayLists 
  WHERE
  wayId = ?;