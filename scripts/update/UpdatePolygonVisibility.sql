--Update when a polygon is used in queries for displaying the map 
UPDATE spt_polygons
SET displayMin = 
    CASE
        WHEN area >= :area  THEN :zoomBand 
    ELSE displayMin
    END
;
