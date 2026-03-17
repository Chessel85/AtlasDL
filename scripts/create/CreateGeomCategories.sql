--Table listing the basic geometry types
CREATE TABLE ref_geometryCategories (
    categoryId INTEGER PRIMARY KEY, -- 1, 2, 3 for point, line and polygon 
    categoryName TEXT NOT NULL       -- 'Point', 'LineString', 'Polygon'
);

INSERT INTO ref_geometryCategories (categoryId, categoryName) VALUES 
(1, 'point'), 
(2, 'linestring'), 
(3, 'polygon');