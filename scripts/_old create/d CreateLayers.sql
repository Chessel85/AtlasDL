--Table to store layers which indicates a type of data downloaded in one go 
CREATE TABLE tbl_layers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    layerName TEXT UNIQUE NOT NULL,
    geomCategoryId INTEGER NOT NULL,
    displayMin INTEGER DEFAULT 1, --display from step size 1 and up
    displayMax INTEGER DEFAULT 20, -- display step size 20 and below 
        FOREIGN KEY (geomCategoryId) REFERENCES ref_geometryCategories(categoryId)
);