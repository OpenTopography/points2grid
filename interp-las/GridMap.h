#ifndef _GRID_MAP_H_
#define _GRID_MAP_H_

using namespace std;

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include "GridFile.h"

class GridMap
{
    public:
	GridMap(int _id, int _size_x, int _lower_bound, int _upper_bound, int _overlap_lower_bound, int _overlap_uuper_bound, bool _initialized, char * fname);
	~GridMap();

    public:
	int getId();
	int getLowerBound();
	int getUpperBound();
	int getOverlapLowerBound();
	int getOverlapUpperBound();
	GridFile *getGridFile();

	bool isInitialized();

	void setId(int _id);
	void setLowerBound(int _lower_bound);
	void setUpperBound(int _upper_bound);
	void setOverlapLowerBound(int _overlap_lower_bound);
	void setOverlapUpperBound(int _overlap_upper_bound);
	void setInitialized(bool _initialized);
	//void setGridFile(string fname);

    private:
	int lowerBound;
	int upperBound;
	int overlapLowerBound;
	int overlapUpperBound;

	bool initialized;
	int id;
	GridFile * gridFile;
};

#endif
