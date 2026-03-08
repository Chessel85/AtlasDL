--Select the id for a given geometry name 
SELECT categoryId FROM ref_geometryCategories 
  WHERE categoryName = :geomCategory;