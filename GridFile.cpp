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

#include "GridFile.h"
#include <float.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

GridFile::GridFile(int id, char *_fname, int _size_x, int _size_y)
{
    FILE *fp;
    size_t n = 0;

    ID = id;
    strncpy(fname, _fname, sizeof(fname));
    size_x = _size_x;
    size_y = _size_y;
    inMemory = false;

    if((fp = fopen64(fname, "w+")) == NULL)
    {
	fprintf(stderr, "%s fopen error %d(%s) \n", fname, errno, strerror(errno));
    }

    GridPoint init_value = {DBL_MAX, -DBL_MAX, 0, 0, 0, 0, 0, 0};

    for(int i = 0; i < size_x * size_y; i++)
	n += fwrite(&init_value, sizeof(GridPoint), 1, fp);

    cout << id << ". file size: " << n * sizeof(GridPoint) << endl;
    fclose(fp);

}

GridFile::~GridFile()
{
    if(inMemory == true){
	munmap(interp, sizeof(GridPoint) * size_x * size_y);
	inMemory = false;
	interp = NULL;
    }

    if(filedes >= 0)
	close(filedes);

    unlink(fname);
}

int GridFile::getId()
{
    return ID;
}

// memory map
int GridFile::map()
{
    if((filedes = open(fname, O_RDWR)) < 0)
    {
	fprintf(stderr, "%s open error %d(%s)\n", fname, errno, strerror(errno));
    }

    if((interp = (GridPoint *)mmap(0, sizeof(GridPoint) * size_x * size_y, PROT_READ | PROT_WRITE, MAP_SHARED, filedes, 0)) == MAP_FAILED)
    {
	fprintf(stderr, "mmap error %d(%s) \n", errno, strerror(errno));
	return -1;
    }
    inMemory = true;

    return 0;
}

int GridFile::unmap()
{
    int rc;
    
    if(interp != NULL)
    {
	rc = munmap(interp, sizeof(GridPoint) * size_x * size_y);

	if(rc < 0)
	{
	    fprintf(stderr, "munmap error %d(%s)\n", errno, strerror(errno));
	    return -1;
	}
	   
	inMemory = false;
	interp = NULL;
	close(filedes);
	filedes = -1;

    }

    return 0;
}

bool GridFile::isInMemory()
{
    return inMemory;
}

unsigned int GridFile::getMemSize()
{
    return size_x * size_y * sizeof(GridPoint);
}
