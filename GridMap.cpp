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


