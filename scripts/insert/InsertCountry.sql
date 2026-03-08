--Insert as much country data as we can 
INSERT OR IGNORE INTO tbl_countries  ( 
    relationId,
    name,
    minX, minY, maxX, maxY,
    labelX, labelY,
    midX, midY
) 
VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ? );
