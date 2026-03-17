--Table to store layers 
CREATE TABLE tbl_layers (
    layerId INTEGER PRIMARY KEY AUTOINCREMENT,
    layerName TEXT UNIQUE NOT NULL,
    geomCategoryId INTEGER NOT NULL,
    selected INTEGER DEFAULT 0, --user has selected this layer 
    displayOrder INTEGER DEFAULT NULL, --displayOrder 1 is at the bottom  
    display INTEGER DEFAULT 0, --+1 means display and 0 means do not draw 
        FOREIGN KEY (geomCategoryId) REFERENCES ref_geometryCategories(categoryId)
);
--Trigger to autofill displayOrder when a new layer is added with max( displayOrder ) + 1 

CREATE TRIGGER trg_tbl_layers_displayOrder
  AFTER INSERT ON tbl_layers
  FOR EACH ROW
  WHEN NEW.displayOrder IS NULL
  BEGIN
    UPDATE tbl_layers SET displayOrder = (
        SELECT IFNULL(MAX(displayOrder), 0) + 1 
        FROM tbl_layers
    )
    WHERE layerId = NEW.layerId;
END;