--It is possible that null polygons get into the spatial polygon table so delete these and references to them in other tables
--TABLE tbl_layerPolygons ( has a cascade delete so no action needed
--tbl_polygonOwners also has a cascade delete so no action needed 
--tbl_wayLists also has a delete cascade 
--So just need to delete the polygons
DELETE FROM spt_polygons where polygon1 IS NULL;

