-- Add in base layer groups 
INSERT INTO tbl_layerGroups  ( layerGroupName, selected, isBase )
  VALUES
    ( 'Political', 1, 1 ),  -- selected and is base layer group 
    ( 'Oceans and seas', 1, 1 ), --is selected and is base layer group 
    ( 'Water features', 1, 0 ), --is selected but is not a base layer group 
    ( 'Populated places', 1, 0 ); -- is selected but not a base layer group 
