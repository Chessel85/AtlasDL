--Trigger to delete entries in tbl_links table when point, linestring or polygon geometry is deleted 
CREATE TRIGGER tgr_cleanupLinksPoint
AFTER DELETE ON spt_points
FOR EACH ROW
BEGIN
    DELETE FROM tbl_links 
    WHERE 
        -- Check if the deleted point was the source
        (geomId1 = OLD.pointId AND layerId1 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 1
        ))
        OR 
        -- Check if the deleted point was the target
        (geomId2 = OLD.pointId AND layerId2 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 1
        ));
END;

CREATE TRIGGER tgr_cleanupLinksLinestring
AFTER DELETE ON spt_linestrings
FOR EACH ROW
BEGIN
    DELETE FROM tbl_links 
    WHERE 
        -- Check if the deleted line was the source
        (geomId1 = OLD.lineId AND layerId1 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 2
        ))
        OR 
        -- Check if the deleted line was the target
        (geomId2 = OLD.lineId AND layerId2 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 2
        ));
END;

CREATE TRIGGER tgr_cleanupLinksPolygons
AFTER DELETE ON spt_polygons
FOR EACH ROW
BEGIN
    DELETE FROM tbl_links 
    WHERE 
        -- Case 1: The deleted polygon was the "Source" (geomId1)
        (geomId1 = OLD.polygonId AND layerId1 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 3
        ))
        OR 
        -- Case 2: The deleted polygon was the "Target" (geomId2)
        (geomId2 = OLD.polygonId AND layerId2 IN (
            SELECT id FROM tbl_layers WHERE geomCategoryId = 3
        ));
END;