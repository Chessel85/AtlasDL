--Table to store the types of relationship or link between geometries according to OGC standard 
CREATE TABLE ref_linkTypes (
    linkTypeId INTEGER PRIMARY KEY,
    typeName   TEXT NOT NULL,
    description TEXT
);

--Populate table 
INSERT INTO ref_linkTypes (linkTypeId, typeName, description) VALUES 
-- 1-9: Core OGC Containment/Proximity
    (1, 'Contains', 'Geometry 1 entirely encloses Geometry 2'),
    (2, 'Within', 'Geometry 1 is entirely inside Geometry 2'),
    (3, 'Intersects', 'Geometries share any space but one does not fully contain the other'),
    (4, 'Disjoint', 'Geometries share no space at all'),

-- 10-19: Adjacency (The "Touches" Family)
    (10, 'Borders', 'Polygons share a boundary line'),
    (11, 'PolygonTouches', 'Polygons touch at a single point'),
    (12, 'PointTouches', 'A point sits exactly on a linestring'),
    (13, 'PointOnPerimeter', 'A point sits exactly on a polygon perimeter'),

-- 20-29: Crossing and Overlapping
    (20, 'Crosses', 'A line passes through a polygon or another line'),
    (21, 'Overlaps', 'Two geometries of same dimension share some area/length'),

-- 30-39: Logical/Network (Optional)
    (30, 'Connected', 'Nodes part of the same stitched network'),
    (31, 'Clustered', 'Features belong to the same Hull or grouping');