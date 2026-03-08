--Polygon table contains layer id and colour index to improve performance when rendering 
--Need to do this specifically for top level country polygons 
SELECT layerId, colourIndex, COUNT(*) 
FROM spt_polygons 
GROUP BY layerId, colourIndex;