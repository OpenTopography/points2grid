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

    // for each file, you have to initialize every points
    // then map.initialized = true;
    if((fp = fopen64(fname, "w+")) == NULL)
    {
	fprintf(stderr, "%s fopen error %d(%s) \n", fname, errno, strerror(errno));
    }

    GridPoint init_value = {DBL_MAX, -DBL_MAX, 0, 0, 0, 0, 0, 0};

    for(int i = 0; i < size_x * size_y; i++)
	n += fwrite(&init_value, sizeof(GridPoint), 1, fp);

    cout << id << ". file size: " << n * sizeof(GridPoint) << endl;
    fclose(fp);

    /*
    if((filedes = open(fname, O_RDWR)) < 0)
    {
	fprintf(stderr, "%s open error %d(%s)\n", fname, errno, strerror(errno));
    }
    */

    //cout << "file initialized: " << ID << endl;
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

    //cout << "file closed: " << ID << endl;
}

int GridFile::getId()
{
    return ID;
}

// memory map
int GridFile::map()
{
    //filedes = fileno(fp);
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

    //cout << "map size: " << sizeof(GridPoint) * size_x * size_y << endl;
    //cout << "sizeof(GridPoint): " << sizeof(GridPoint) << endl;
    //cout << "mmaped: " << ID << endl;
}

int GridFile::unmap()
{
    int rc;

    // have to delete previous information??
    // have to overwrite

    // we can track the changes but that scheme requires memory usage.
    // But our main goal is to fully utilize memory. 
    
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

	//cout << "unmapped: " << ID << endl;
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
