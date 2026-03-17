--Table to store what relation or way a polygon geometry belongs to 
CREATE TABLE tbl_polygonOwners (
    polygonId INTEGER NOT NULL,
    elementType INTEGER NOT NULL,
    elementId INTEGER NOT NULL,
    FOREIGN KEY( polygonId ) 
        REFERENCES spt_polygons( polygonId ) 
        ON DELETE CASCADE,
    UNIQUE( polygonId, elementType, elementId )
);

--Create index on polygonId and elementId to speed up visible polygons query
CREATE INDEX idx_tbl_polygonOwners_polygonId ON tbl_polygonOwners( polygonId );
CREATE INDEX idx_tbl_polygonOwners_elementId ON tbl_polygonOwners( elementId );