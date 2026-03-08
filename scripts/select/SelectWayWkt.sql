--return the wkt for a way geometry and reverse if required by the parameter (1=reverse)
SELECT
  CASE :reverseFlag
    WHEN 1 THEN ST_AsText(ST_Reverse( ??? ))
    ELSE ST_AsText( ??? )
  END AS wkt_output
  FROM spt_ways 
  WHERE wayId = :wayId;