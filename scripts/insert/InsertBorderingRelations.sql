--Series of statements that very quickly works out countries that border each other based on admin level 6 data
BEGIN;

-- 1. Create a lightning-fast lookup for Polygon -> Sovereign Relation ID
-- This maps every Level 6 (or any) polygon directly to its Layer 1 Country ID
CREATE TEMP TABLE tmp_poly_to_sovereign AS
  SELECT lp_child.polygonId, po_parent.elementId
  FROM tbl_layerPolygons AS lp_child
  JOIN tbl_links AS l ON lp_child.polygonId = l.geomId2 AND l.linkType = 1 -- 'Contains'
  JOIN tbl_layerPolygons AS lp_parent ON l.geomId1 = lp_parent.polygonId
  JOIN tbl_polygonOwners AS po_parent ON lp_parent.polygonId = po_parent.polygonId
  WHERE lp_parent.layerId = 1;

-- 2. Index the temp table to ensure the main join is O(log n)
CREATE INDEX idx_tmp_pts_poly ON tmp_poly_to_sovereign(polygonId);

-- 3. Perform the bulk insert using only integers
-- We use 'linkType = 10' for the Level 6 shared ways 
INSERT OR IGNORE INTO tbl_Borders (fromRelationID, toRelationID, maritime)
  SELECT DISTINCT
    m1.elementId,
    m2.elementId,
    0
  FROM tbl_links AS l
  JOIN tmp_poly_to_sovereign AS m1 ON l.geomId1 = m1.polygonId
  JOIN tmp_poly_to_sovereign AS m2 ON l.geomId2 = m2.polygonId
  WHERE l.linkType = 10 
    AND m1.elementId != m2.elementId;

-- 4. Clean up: Populate the Names once the ID pairs are established
-- This runs once per country pair rather than once per border segment
UPDATE tbl_Borders 
  SET fromRelationName = (
        SELECT COALESCE(
            (SELECT value FROM tbl_tags WHERE elementId = fromRelationID AND key = 'name:en'),
            (SELECT value FROM tbl_tags WHERE elementId = fromRelationID AND key = 'name')
        )
    ),
    toRelationName = (
        SELECT COALESCE(
            (SELECT value FROM tbl_tags WHERE elementId = toRelationID AND key = 'name:en'),
            (SELECT value FROM tbl_tags WHERE elementId = toRelationID AND key = 'name')
        )
    );

-- 5. Final cleanup of the temp assets
DROP TABLE tmp_poly_to_sovereign;

COMMIT;