--Insert a single tag 
INSERT OR IGNORE INTO tbl_Tags  
  ( 
    elementType,
    elementID,
    key,
    value
  ) 
  VALUES ( ?, ?, ?, ? );