-- Calculate minZoomLevel and minLabelZoomLevel for a polygon
WITH Constants AS (
    SELECT 
        4.0 AS threshold,      -- Minimum pixels for display
        7.0 AS labelThreshold, -- threshold for when to display the polygon label 
        1920 AS screenWidth,   
        1080 AS screenHeight,  
        20000 AS worldWidth,   
        111.34 AS degToKm 
)

UPDATE spt_polygons
SET 
    minZoomLevel = (
        SELECT 
            MAX(
                log( (c.worldWidth * c.threshold) / (abs(ST_MaxX(polygon) - ST_MinX(polygon)) * (c.degToKm * cos( ((ST_MaxY(polygon) + ST_MinY(polygon)) / 2.0) * (pi() / 180.0) )) * c.screenWidth) ) / log(2),
                log( (c.worldWidth * c.threshold) / (abs(ST_MaxY(polygon) - ST_MinY(polygon)) * c.degToKm * c.screenHeight) ) / log(2)
            )
        FROM Constants AS c
    ),
    minLabelZoomLevel = (
        SELECT 
            MAX(
                log( (c.worldWidth * c.labelThreshold) / (abs(ST_MaxX(polygon) - ST_MinX(polygon)) * (c.degToKm * cos( ((ST_MaxY(polygon) + ST_MinY(polygon)) / 2.0) * (pi() / 180.0) )) * c.screenWidth) ) / log(2),
                log( (c.worldWidth * c.labelThreshold) / (abs(ST_MaxY(polygon) - ST_MinY(polygon)) * c.degToKm * c.screenHeight) ) / log(2)
            )
        FROM Constants AS c
    );