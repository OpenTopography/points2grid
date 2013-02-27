/*
   Local Interpolation Implementation

   1. Grid file index structure 
   2. Out-of-core Implementation

   3. no streaming
   4. no parallelization
   Based on the notes by Prof. Ramon Arrowsmith(ramon.arrowsmith@asu.edu)
*/

#ifndef _INTERP_H_
#define _INTERP_H_

#include <string>
#include <iostream>

using namespace std;

#include "GridPoint.h"
#include "CoreInterp.h"
#include "OutCoreInterp.h"
#include "InCoreInterp.h"

enum coreMode
{
    INCORE = 0,
    OUTCORE
};

//class GridPoint;

class Interpolation
{
    public:
	Interpolation(double x_dist, double y_dist, double radius, int _window_size);
	~Interpolation();

	int init(char *inputName, int inputFormat);

	int interpolation(char *inputName, char *outputName, int inputFormat, 
			  int outputFormat, unsigned int type);

	unsigned int getDataCount();
	
	unsigned int getGridSizeX();
	unsigned int getGridSizeY();

	// for debug
	void printArray();
	
	// depricated
	void setRadius(double r);

    public:
	double GRID_DIST_X;
	double GRID_DIST_Y;

	static const int MAX_POINT_SIZE = 16000000;
	static const double WEIGHTER = 2.0;
	//static const unsigned int MEM_LIMIT = 31000000;
	//static const unsigned int MEM_LIMIT = 600000000;
	//static const unsigned int MEM_LIMIT = 120000000;
	// static const unsigned int MEM_LIMIT =     100000;
	static const unsigned int MEM_LIMIT = 200000000;

    private:
	//double arrX[MAX_POINT_SIZE];
	//double arrY[MAX_POINT_SIZE];
	//double arrZ[MAX_POINT_SIZE];

	double min_x;
	double min_y;
	double max_x;
	double max_y;

	unsigned int GRID_SIZE_X;
	unsigned int GRID_SIZE_Y;

	unsigned int data_count;
	double radius_sqr;
	int window_size;

	int core_mode;

	CoreInterp *interp;
};

#endif
