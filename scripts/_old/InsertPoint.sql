--Insert a row into points table with a point geometry 
INSERT INTO spt_Points ( 
 nodeID, 
   point
) 
 VALUES ( ?, MakePoint( ?,?,  4326 ) ); 
