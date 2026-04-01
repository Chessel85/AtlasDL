--Table to store layer groups like admin levels, water
CREATE TABLE tbl_layerGroups (
    layerGroupId INTEGER PRIMARY KEY AUTOINCREMENT,
    layerGroupName TEXT UNIQUE NOT NULL,
    selected INTEGER DEFAULT 0, --user has selected this layer  group
    isBase INTEGER DEFAULT 0 --contained layers are base layers where at least one must be selected 
);

