--As part of moving a polygon from one layer to another then delete it first
DELETE FROM tbl_layerPolygons  WHERE layerId = ? AND polygonId = ?;

