--Output tbl_AdminRelations to csv 
.headers on
.mode csv
.output adminRelations.csv
SELECT * FROM tbl_AdminRelations ORDER BY relationName;