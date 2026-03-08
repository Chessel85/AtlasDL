# Atlas DL Description 

## Document Purpose 

Atlas DL is a console application that generates a Spatialite database that can then be used by Accessible Atlas.  This document describes the database schema and how it is populated.

## Key Points

The ethos of Accessible Atlas is to use Openstreetmap data:
* It is free
* Very detailed in many ways

The downside is that preparing the data so it can be used is difficult.  The primary example of this is how sovereign state boundaries include territorial waters and removing the excess territory is complex.

## Spatialite 

Spatialite is based on SQLite but builds upon well-optimized DLL's such as PROJDB to bring powerful spatial capabilities.

## OSM data


| Element | Description |
|---|---|
| node | A point with a unique node ID and a coordinate in longitude and latitude |
| way | A series of nodes with a unique way ID |
| relation | A collection of nodes, ways and other relations to represent complex features |

