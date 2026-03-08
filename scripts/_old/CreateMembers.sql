--Table to store relation members 

CREATE TABLE tbl_Members (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  relationID INTEGER,
  type TEXT,
  ref  INTEGER,
  role TEXT
);
