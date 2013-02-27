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
