--Delete a specific polygon possibly done as part of removing territorial waters where original bloated polygons are replaced with trimmed down ones 
--TABLE tbl_layerPolygons ( has a cascade delete so no action needed
--tbl_polygonOwners also has a cascade delete so no action needed 
--tbl_wayLists also has a delete cascade 
--So just need to delete the polygons
DELETE FROM spt_polygons where polygonId = :polygonId;

