--Table to list ways and how they can be constructed to form multilinestrings 
--The geometries for a way are stored separately 
CREATE TABLE tbl_linestringLists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    lineId INTEGER,        -- Foreign key to spt_linestrings
    lineNumber INTEGER, --the line in the multilinestring is zero based 
    sequenceNumber INTEGER, -- Order of the ways in the chain is zero based 
    direction INTEGER,      -- 1 = Forward, -1 = Backward
    wayId INTEGER,          -- Foreign key to spt_ways
    FOREIGN KEY (lineId) REFERENCES spt_linestrings (lineId) ON DELETE CASCADE
);