/*
*
COPYRIGHT AND LICENSE

Copyright (c) 2011 The Regents of the University of California.
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.

3. All advertising materials mentioning features or use of this
software must display the following acknowledgement: This product
includes software developed by the San Diego Supercomputer Center.

4. Neither the names of the Centers nor the names of the contributors
may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*
*
* Based on the notes by Prof. Ramon Arrowsmith(ramon.arrowsmith@asu.edu)
* Authors: Han S Kim (hskim@cs.ucsd.edu), Sriram Krishnan (sriram@sdsc.edu)
*
*/

#include "Interpolation.h"
#include "Global.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <string.h>
#include <fstream>  // std::ifstream
#include <iostream> // std::cout
#ifdef USE_LIBLAS_17
	//if binding against liblas 1.7+ API, use C API calls
	//(backported from CRREL codebase)
	extern "C" {
		#include <liblas/capi/liblas.h> // the C API is stable across liblas versions
	}
#else
	//use liblas 1.2.1 C++ API by default
	#include <liblas/laspoint.hpp>
	#include <liblas/lasreader.hpp>
#endif


/////////////////////////////////////////////////////////////
// Public Methods
/////////////////////////////////////////////////////////////

//x_dist and y_dist represent length and height of each grid cell (resolution)
//radius represents search radius.
//window size represents fill window size
Interpolation::Interpolation(double x_dist, double y_dist, double radius, int _window_size) : GRID_DIST_X (x_dist), GRID_DIST_Y(y_dist)
{
    data_count = 0;
    radius_sqr = radius * radius;
    window_size = _window_size;

    min_x = DBL_MAX;
    min_y = DBL_MAX;

    max_x = -DBL_MAX;
    max_y = -DBL_MAX;
}

Interpolation::~Interpolation()
{
    delete interp;
}
	
int Interpolation::init(char *inputName, int inputFormat)
{
    //unsigned int i;

    //struct tms tbuf;
    clock_t t0, t1;


    //////////////////////////////////////////////////////////////////////
    // MIN/MAX SEARCHING
    // TODO: the size of data, min, max of each coordinate 
    //       are required to implement streaming processing....
    //
    // This code can be eliminated if database can provide these values
    //////////////////////////////////////////////////////////////////////

    //t0 = times(&tbuf);
    t0 = clock();

    if(inputName == NULL)
	{
	    cout << "Wrong Input File Name" << endl;
	    return -1;
	}

    printf("inputName: '%s'\n", inputName);

    if (inputFormat == INPUT_ASCII) {
	FILE *fp;
	char line[1024];
	double data_x, data_y;
	//double data_z;

	if((fp = fopen64(inputName, "r")) == NULL)
	    {
		cout << "file open error" << endl;
		return -1;
	    }
      
	// throw the first line away - it contains the header
	fgets(line, sizeof(line), fp);
      
	// read the data points to find min and max values
	while(fgets(line, sizeof(line), fp) != NULL)
	    {
		data_x = atof(strtok(line, ",\n"));
		if(min_x > data_x) min_x = data_x;
		if(max_x < data_x) max_x = data_x;
	  
		data_y = atof(strtok(NULL, ",\n"));
		if(min_y > data_y) min_y = data_y;
		if(max_y < data_y) max_y = data_y;
	  
		data_count++;
	  
	  
		/*
		// tokenizing
		string strLine(line);
	  
		// first token
		string::size_type pos = strLine.find_first_of(",",0);
		string::size_type lastPos = strLine.find_first_not_of(",",0);
	  
		string strX = strLine.substr(lastPos, pos - lastPos);
	  
		// second token
		lastPos = strLine.find_first_not_of(",", pos);
		pos = strLine.find_first_of(",", lastPos);
	  
		string strY = strLine.substr(lastPos, pos - lastPos);
	  
		// third token
		lastPos = strLine.find_first_not_of(",", pos);
		pos = strLine.find_first_of("\n", lastPos);
	  
		string strZ = strLine.substr(lastPos, pos - lastPos);
	  
		// data conversion
		arrX[data_count] = atof(strX.c_str());
		if(min_x > arrX[data_count]) min_x = arrX[data_count];
		if(max_x < arrX[data_count]) max_x = arrX[data_count];
	  
		arrY[data_count] = atof(strY.c_str());
		if(min_y > arrY[data_count]) min_y = arrY[data_count];
		if(max_y < arrY[data_count]) max_y = arrY[data_count];
	  
		arrZ[data_count++] = atof(strZ.c_str());
		*/
	    }
      
	fclose(fp);
    } else { // las input

#ifdef USE_LIBLAS_17
        LASReaderH lr = LASReader_Create(inputName);
        if (!lr) {
            LASError_Print("Could not open file to read.");
            return -1;
        }

        LASHeaderH lh = LASReader_GetHeader(lr);
        if (!lh) {
            LASError_Print("error while reading LAS file: verify that the input is valid LAS file.");
            LASReader_Destroy(lr);
            return -1;
        }

        min_x = LASHeader_GetMinX(lh);
        min_y = LASHeader_GetMinY(lh);
        max_x = LASHeader_GetMaxX(lh);
        max_y = LASHeader_GetMaxY(lh);
        data_count = LASHeader_GetPointRecordsCount(lh);

        LASReader_Destroy(lr);
#else
	try {
	    std::ifstream ifs;
	    ifs.open(inputName, std::ios::in | std::ios::binary);
	    
	    // create a las file reader
	    liblas::LASReader reader(ifs);
	    
	    /// get header information
	    liblas::LASHeader const& header = reader.GetHeader();
	    
	    min_x = header.GetMinX();
	    min_y = header.GetMinY();
	    max_x = header.GetMaxX();
	    max_y = header.GetMaxY();
	    data_count = header.GetPointRecordsCount();
	    
	    ifs.close();
	} catch (std::runtime_error &e) {
	    cout << "error while reading LAS file: verify that the input is valid LAS file" << endl;
	    return -1;
	}
#endif
    }

    t1 = clock();
    //t1 = times(&tbuf);
    //printf("Min/Max searching time: %10.2f\n", (t1 - t0)/(double) sysconf(_SC_CLK_TCK));
    printf("Min/Max searching time: %10.2f\n", (double)(t1 - t0)/CLOCKS_PER_SEC);


    //t0 = times(&tbuf);

    //////////////////////////////////////////////////////////////////////
    // Intialization Step excluding min/max searching
    //////////////////////////////////////////////////////////////////////
    /*
      for(i = 0; i < data_count; i++)
      {
      arrX[i] -= min_x;
      arrY[i] -= min_y;
      //printf("%f,%f,%f\n", arrX[i] , arrY[i] ,arrZ[i]);
      }
    */

#ifdef ALIGN_ORIGIN
	cout.flags ( ios::fixed);
	cout.width (10);

	cout << "Bounding Box:\t(" << min_x << ", " << min_y <<  "), (" << max_x << ", " << max_y << ")\n";

	//a bounding box/grid based on min/max x and y values is not likely to be aligned to origin.
	//to align to origin, the origin must be a grid point, and all other grid points must be
	//multiples of the grid resolution (GRID_DIST_X/Y).

	//By default, p2g makes the lower-left corner of the bounding box of the points (min_x, min_y)
	//the lower-left corner of the grid, and the upper-right corner of the grid will be some multiple
	//of GRID_DISTX/Y that will encompass (max_x,max_y).

	//We align the grid to origin by extending the lower-left corner of the bounding box south-west
	//until it aligns with the closest grid-point. The new lower-left corner will become the new min_x, min_y.
	//this works because min/max values are only used to determine the size of the grid; interp does not
	//require a point to exist at min_x and/or min_y. Also, because interp iterates through the point-cloud,
	//padding the grid does not impact DEM generation time.

	min_x = floor(min_x/GRID_DIST_X) * GRID_DIST_X;
	min_y = floor(min_y/GRID_DIST_Y) * GRID_DIST_Y;

	//max_x and max_y record the true upper-right boundary of the point-cloud and are not similarly padded.
	//The upper-right corner of the grid will be aligned to origin, being an multiple of GRID_DIST_X/Y.
	GRID_SIZE_X = (int)((max_x - min_x)/GRID_DIST_X) + 1;
	GRID_SIZE_Y = (int)((max_y - min_y)/GRID_DIST_Y) + 1;

	double grid_max_x = GRID_SIZE_X * GRID_DIST_X + min_x;
	double grid_max_y = GRID_SIZE_Y * GRID_DIST_Y + min_y;

	cout << "Aligned Grid:\t(" << min_x << ", " << min_y <<  "), (" << grid_max_x << ", " << grid_max_y << ")\n";
#else
	cout << "ORIGINAL\tmin_x: " << min_x << ", max_x: " << max_x << ", min_y: " << min_y << ", max_y: " << max_y << endl;
	GRID_SIZE_X = (int)((max_x - min_x)/GRID_DIST_X) + 1;
	GRID_SIZE_Y = (int)((max_y - min_y)/GRID_DIST_Y) + 1;
#endif

	cout << "GRID_SIZE_X " << GRID_SIZE_X << endl;
	cout << "GRID_SIZE_Y " << GRID_SIZE_Y << endl;
	cout << "NUMBER OF GRID POINTS: " << GRID_SIZE_X * GRID_SIZE_Y << "\n";

#ifdef HARD_LIMIT
	if((GRID_SIZE_X * GRID_SIZE_Y) > HARD_LIMIT)
	{
		cout << "size of grid exceeds hard limit. Aborting...\n";
		return -1;
	}
#endif


    // if the size is too big to fit in memory,
    // then 
    //	construct out-of-core structure
    if(GRID_SIZE_X * GRID_SIZE_Y > MEM_LIMIT)
	{
	    core_mode = OUTCORE;

	    interp = new OutCoreInterp(GRID_DIST_X, GRID_DIST_Y, GRID_SIZE_X, GRID_SIZE_Y, radius_sqr, min_x, max_x, min_y, max_y, window_size);
	    if(interp == NULL)
		{
		    cout << "OutCoreInterp construction error" << endl;
		    return -1;
		}

	    cout << "Interpolation uses out-of-core algorithm" << endl;

	    // else
	    //	do normal
	} else {
	core_mode = INCORE;

	interp = new InCoreInterp(GRID_DIST_X, GRID_DIST_Y, GRID_SIZE_X, GRID_SIZE_Y, radius_sqr, min_x, max_x, min_y, max_y, window_size);

	cout << "Interpolation uses in-core algorithm" << endl;
    }

    if(interp->init() < 0)
	{
	    cout << "inter->init() error" << endl;
	    return -1;
	}
	
    cout << "Interpolation::init() done successfully" << endl;

    //t1 = times(&tbuf);
    //printf("Initializing time: %10.2f\n", (t1 - t0)/(double) sysconf(_SC_CLK_TCK));

    return 0;
}

int Interpolation::interpolation(char *inputName, 
				 char *outputName, 
				 int inputFormat, 
				 int outputFormat, 
				 unsigned int outputType)
{
    int rc;
    //unsigned int i;
    double data_x, data_y;
    double data_z;

    //struct tms tbuf;
    //clock_t t0, t1;

    printf("Interpolation Starts\n");

    //t0 = times(&tbuf);

    //cout << "data_count: " << data_count << endl;

    /*
      if((rc = interp->init()) < 0)
      {
      cout << "inter->init() error" << endl;
      return -1;
      }
    */

    if (inputFormat == INPUT_ASCII) {
	FILE *fp;
	char line[1024];

	if((fp = fopen64(inputName, "r")) == NULL)
	    {
		printf("file open error\n");
		return -1;
	    }
      
	// throw the first line away - it contains the header
	fgets(line, sizeof(line), fp);
      
	// read every point and generate DEM
	while(fgets(line, sizeof(line), fp) != NULL)
	    {
		data_x = atof(strtok(line, ",\n"));
		data_y = atof(strtok(NULL, ",\n"));
		data_z = atof(strtok(NULL, ",\n"));
	  
		data_x -= min_x;
		data_y -= min_y;
	  
		//if((rc = interp->update(arrX[i], arrY[i], arrZ[i])) < 0)
		if((rc = interp->update(data_x, data_y, data_z)) < 0)
		    {
			cout << "interp->update() error while processing " << endl;
			return -1;
		    }
	    }
      
	fclose(fp);
    } else { // input format is LAS
#ifdef USE_LIBLAS_17
        LASReaderH lr = LASReader_Create(inputName);
        if (!lr) {
            LASError_Print("Could not open file to read.");
            return -1;
        }

        LASPointH lp;
        while ((lp = LASReader_GetNextPoint(lr))) {
            data_x = LASPoint_GetX(lp);
            data_y = LASPoint_GetY(lp);
            data_z = LASPoint_GetZ(lp);

            data_x -= min_x;
            data_y -= min_y;

            if ((rc = interp->update(data_x, data_y, data_z)) < 0) {
                cout << "interp->update() error while processing " << endl;
                return -1;
            }
        }

        LASReader_Destroy(lr);
#else
	// instantiate a reader for the LAS file
	std::ifstream ifs;
	ifs.open(inputName, std::ios::in | std::ios::binary);

	liblas::LASReader reader(ifs);

	// process every point in the LAS file, and generate DEM
	while (reader.ReadNextPoint()) {
	    liblas::LASPoint const& p = reader.GetPoint();

	    data_x = p.GetX(); 
	    data_y = p.GetY();
	    data_z = p.GetZ();

	    data_x -= min_x;
	    data_y -= min_y;

	    //if((rc = interp->update(arrX[i], arrY[i], arrZ[i])) < 0)                                               
	    if((rc = interp->update(data_x, data_y, data_z)) < 0)
		{
		    cout << "interp->update() error while processing " << endl;
		    return -1;
		}
	}
      
	ifs.close();
#endif
    }

    if((rc = interp->finish(outputName, outputFormat, outputType)) < 0)
	{
	    cout << "interp->finish() error" << endl;
	    return -1;
	}

    cout << "Interpolation::interpolation() done successfully" << endl;

    //t1 = times(&tbuf);
    //printf("Interpolation + Output time: %10.2f\n", (t1 - t0)/(double) sysconf(_SC_CLK_TCK));

    return 0;
}

void Interpolation::setRadius(double r)
{
    radius_sqr = r * r;
}

unsigned int Interpolation::getDataCount()
{
    return data_count;
}

unsigned int Interpolation::getGridSizeX()
{
    return GRID_SIZE_X;
}

unsigned int Interpolation::getGridSizeY()
{
    return GRID_SIZE_Y;
}

