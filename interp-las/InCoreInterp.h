#ifndef _IN_CORE_INTERP_H_
#define _IN_CORE_INTERP_H_

using namespace std;

#include <iostream>
#include "GridPoint.h"
#include "CoreInterp.h"

class InCoreInterp : public CoreInterp
{
 public:
    InCoreInterp() {};
    InCoreInterp(double dist_x, double dist_y, 
		 int size_x, int size_y, 
		 double r_sqr,
		 double _min_x, double _max_x,
		 double _min_y, double _max_y,
		 int _window_size);
    ~InCoreInterp();

    virtual int init();
    virtual int update(double data_x, double data_y, double data_z);
    virtual int finish(char *outputName, int outputFormat, unsigned int outputType);

 private:
    GridPoint **interp;
    double radius_sqr;

 private:
    void update_first_quadrant(double data_z, int base_x, int base_y, double x, double y);
    void update_second_quadrant(double data_z, int base_x, int base_y, double x, double y);
    void update_third_quadrant(double data_z, int base_x, int base_y, double x, double y);
    void update_fourth_quadrant(double data_z, int base_x, int base_y, double x, double y);

    void updateGridPoint(int x, int y, double data_z, double distance);
    void printArray();
    int outputFile(char *outputName, int outputFormat, unsigned int outputType);
};

#endif
