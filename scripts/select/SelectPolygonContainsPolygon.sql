--Return 1 if the second polygon in WKT format is completely  enclosed within first polygon in WKT format 
--Returns 1 if they touch  or have a shared segment and only returns 0 if a point is actually outside 
SELECT ST_Contains(
  ( SELECT
    BuildArea( ST_Collect( collected_way ) )
  FROM
  (
    SELECT
        spt_ways.way1 AS collected_way  
    FROM 
        spt_ways 
    WHERE
        spt_ways.wayId IN ??? 
    ) ),
  ( SELECT
    BuildArea( ST_Collect( collected_way ) )
  FROM
  (
    SELECT
        spt_ways.way1 AS collected_way  
    FROM 
        spt_ways 
    WHERE
        spt_ways.wayId IN ??? 
    ) )
  );