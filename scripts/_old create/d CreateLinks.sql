--Table to store relationships between geometries 
CREATE TABLE tbl_links (
    layerId1    INTEGER NOT NULL,
    geomId1     INTEGER NOT NULL,
    linkType    INTEGER NOT NULL,
    layerId2    INTEGER NOT NULL,
    geomId2     INTEGER NOT NULL,
    intersectionDimension INTEGER DEFAULT 0, 
    intersectionSize REAL DEFAULT 0,
    PRIMARY KEY (layerId1, geomId1, layerId2, geomId2, linkType),
    FOREIGN KEY (layerId1) REFERENCES tbl_layers(id) ON DELETE CASCADE,
    FOREIGN KEY (layerId2) REFERENCES tbl_layers(id) ON DELETE CASCADE,
    FOREIGN KEY (linkType) REFERENCES ref_linkTypes(linkTypeId)
);

--Create indexes on geomId1 and geomId2  to speed up getting visible polygons 
CREATE INDEX idx_tbl_links_geomId1 ON tbl_links(geomId1);
CREATE INDEX idx_tbl_links_geomId2 ON tbl_links(geomId2);