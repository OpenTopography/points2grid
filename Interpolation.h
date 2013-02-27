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

   Local Interpolation Implementation

   1. Grid file index structure 
   2. Out-of-core Implementation
   3. no streaming
   4. no parallelization

*
*
* Based on the notes by Prof. Ramon Arrowsmith(ramon.arrowsmith@asu.edu)
* Authors: Han S Kim (hskim@cs.ucsd.edu), Sriram Krishnan (sriram@sdsc.edu)
*
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
