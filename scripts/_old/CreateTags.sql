--Table to store tags for nodes, ways and relations 
CREATE TABLE tbl_Tags (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  elementType INTEGER,
  elementID INTEGER,
  key TEXT,
  value TEXT
);

--Create index based on elementID and key  as this combination is used in queries 
CREATE INDEX idx_tbl_tags_elementID_key ON tbl_Tags (elementID, key);
