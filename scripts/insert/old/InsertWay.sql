--Insert a single way into the spt_ways table ignoring any existing way id entries
INSERT OR IGNORE INTO spt_ways (wayId, firstNodeId, lastNodeId, way) 
  VALUES (
    wayId,
    firstNodeId,
    lastNodeId,
    ST_GeomFromText('', 4326)
);