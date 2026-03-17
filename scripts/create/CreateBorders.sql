--Table to store polygons that intersect in the same layer 
CREATE TABLE tbl_borders (
    polygon1Id INTEGER,
    polygon2Id INTEGER,
    name1 TEXT,
    name2 TEXT
);
