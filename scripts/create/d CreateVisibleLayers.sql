-- Table to store user's current layer selection
CREATE TABLE tbl_visibleLayers (
    layerId INTEGER PRIMARY KEY,
    FOREIGN KEY (layerId) REFERENCES tbl_layers(id) ON DELETE CASCADE     -- We use a Foreign Key to ensure we only enable layers that actually exist
);