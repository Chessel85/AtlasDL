--Insert many ways into the spt_ways table ignoring any existing way id entries
INSERT OR IGNORE INTO spt_ways (wayId, firstNodeId, lastNodeId, way)
  VALUES ( ? );
