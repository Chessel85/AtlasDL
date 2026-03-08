--Table to store tags for nodes, ways and relations 
CREATE TABLE tbl_tags (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  elementType INTEGER,
  elementId INTEGER,
  key TEXT,
  value TEXT,
  CONSTRAINT unique_element_tag UNIQUE (elementType, elementId, key)
);
