#ifndef _CORE_INTERP_H_
#define _CORE_INTERP_H_

class CoreInterp
{
    public:
	CoreInterp() {};
	virtual ~CoreInterp() {};

	virtual int init() = 0;
	virtual int update(double data_x, double data_y, double data_z) = 0;
	virtual int finish(char *outputName, int outputFormat, unsigned int outputType) = 0;

    protected:
	double GRID_DIST_X;
	double GRID_DIST_Y;

	int GRID_SIZE_X; // total size of a grid
	int GRID_SIZE_Y; // 

	// for outputting
	double min_x;
	double max_x;
	double min_y;
	double max_y;

	// for DEM filling
	int window_size;
};

#endif
