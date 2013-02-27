#ifndef _GRID_FILE_H_
#define _GRID_FILE_H_

using namespace std;

#include <iostream>
#include <fstream>
#include "GridPoint.h"

class GridFile
{
    public:
	GridFile(int id, char *_fname, int _size_x, int _size_y);
	~GridFile();

	int getId();
	int map();
	int unmap();
	bool isInMemory();
	unsigned int getMemSize();

	GridPoint *interp;

    private:
	//ofstream fout;

	int filedes;
	int ID;
	int size_x;
	int size_y;
	bool inMemory;
	char fname[1024];
};

#endif
