--Update all ways in given column with a given tolerance 
--Need to go via a equidistance cylindrical SRID 4087 to use tolerance in metres  which preserves distances 
UPDATE spt_ways
  SET ??? --will be way2, way3 or way 4
     = ST_Transform( ST_SimplifyPreserveTopology( ST_Transform( way1, 4087), :tolerance ), 4326 );