#ifndef _OUT_CORE_INTERP_H_
#define _OUT_CORE_INTERP_H_

using namespace std;

#include "CoreInterp.h"
#include <string>
#include <iostream>
#include <queue>
#include <list>
#include "GridPoint.h"
#include "GridMap.h"

class UpdateInfo;

class OutCoreInterp : public CoreInterp 
{
 public:
    OutCoreInterp() {};
    OutCoreInterp(double dist_x, double dist_y, 
		  int size_x, int size_y, 
		  double r_sqr,
		  double _min_x, double _max_x,
		  double _min_y, double _max_y,
		  int _window_size);
    ~OutCoreInterp();

    virtual int init();
    virtual int update(double data_x, double data_y, double data_z);
    virtual int finish(char *outputName, int outputFormat, unsigned int outputType);

 private:
    void updateInterpArray(int fileNum, double data_x, double data_y, double data_z);
    void update_first_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y);
    void update_second_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y);
    void update_third_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y);
    void update_fourth_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y);

    void updateGridPoint(int fileNum, int x, int y, double data_z, double distance);
    int findFileNum(double data_y);
    void finalize();
    int outputFile(char *outputName, int outputFormat, unsigned int outputType);
	

 public:
    static const unsigned int QUEUE_LIMIT = 1000;

 private:
    double radius_sqr;

    int overlapSize;
    int local_grid_size_x;
    int local_grid_size_y;

    int numFiles;
    list<UpdateInfo> *qlist;
    GridMap **gridMap;
    int openFile;
};

class UpdateInfo
{
 public:
 UpdateInfo() : data_x(0), data_y(0), data_z(0) {}; 
 UpdateInfo(double x, double y, double z) : data_x(x), data_y(y), data_z(z){};

 public:
    double data_x;
    double data_y;
    double data_z;
};

#endif
