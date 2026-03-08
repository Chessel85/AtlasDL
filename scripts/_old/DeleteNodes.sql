--Delete nodes based on a bunch of node ID passed in 
DELETE FROM spt_Points WHERE
 nodeID IN ( ? );

