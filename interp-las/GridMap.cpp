#include "GridMap.h"
#include <string.h>



	
GridMap::GridMap(int _id, int _size_x, int _lower_bound, int _upper_bound, int _overlap_lower_bound, int _overlap_upper_bound, bool _initialized, char *fname)
{
    id = _id;
    lowerBound = _lower_bound;
    upperBound = _upper_bound;
    overlapLowerBound = _overlap_lower_bound;
    overlapUpperBound = _overlap_upper_bound;
    initialized = _initialized;

    gridFile = new GridFile(id, fname, _size_x, _overlap_upper_bound - _overlap_lower_bound + 1);
}
GridMap::~GridMap()
{
    if(gridFile != NULL)
	delete gridFile;
}

int GridMap::getLowerBound()
{
    return lowerBound;
}
int GridMap::getUpperBound()
{
    return upperBound;
}
int GridMap::getOverlapLowerBound()
{
    return overlapLowerBound;
}
int GridMap::getOverlapUpperBound()
{
    return overlapUpperBound;
}

bool GridMap::isInitialized()
{
    return initialized;
}
int GridMap::getId()
{
    return id;
}
GridFile *GridMap::getGridFile()
{
    return gridFile;
}

void GridMap::setLowerBound(int _lower_bound)
{
    lowerBound = _lower_bound;
}
void GridMap::setUpperBound(int _upper_bound)
{
    upperBound = _upper_bound;
}
void GridMap::setOverlapLowerBound(int _overlap_lower_bound)
{
    overlapLowerBound = _overlap_lower_bound;
}
void GridMap::setOverlapUpperBound(int _overlap_upper_bound)
{
    overlapUpperBound = _overlap_upper_bound;
}
void GridMap::setInitialized(bool _initialized)
{
    initialized = _initialized;
}
void GridMap::setId(int _id)
{
    id = _id;
}
/*
void GridMap::setGridFile(string fname)
{
    if(gridFile != NULL)
	delete gridFile;

    gridFile = new GridFile(id, fname, _size_x, _overlap_upper_bound - _overlap_lower_bound + 1);
}
*/

