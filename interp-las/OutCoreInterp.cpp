#include "OutCoreInterp.h"

#include <float.h>
#include <math.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>
#include "Interpolation.h"
#include "Global.h"
#include <stdio.h>
#include <string.h>

OutCoreInterp::OutCoreInterp(double dist_x, double dist_y, 
                             int size_x, int size_y, 
                             double r_sqr,
                             double _min_x, double _max_x,
                             double _min_y, double _max_y,
			     int _window_size) 
{
    int i;

    GRID_DIST_X = dist_x;
    GRID_DIST_Y = dist_y;

    GRID_SIZE_X = size_x;
    GRID_SIZE_Y = size_y;

    radius_sqr = r_sqr;

    min_x = _min_x;
    max_x = _max_x;
    min_y = _min_y;
    max_y = _max_y;

    window_size = _window_size;

    overlapSize = (int)ceil(sqrt(radius_sqr)/GRID_DIST_Y);
    int window_dist = window_size / 2;
    if (window_dist > overlapSize) {
	overlapSize = window_dist;
    }

    // how many pieces will there be?
    // numFiles = (GRID_SIZE_X*GRID_SIZE_Y + (2 * overlapSize + 1) * GRID_SIZE_X * numFiles) / MEM_LIMIT;
    // (2 * overlapSize + 1) means, each component has one more row to handle the points in-between two components.
    numFiles = (int)ceil((double)GRID_SIZE_X * GRID_SIZE_Y / (Interpolation::MEM_LIMIT - (2 * overlapSize + 1) * GRID_SIZE_X));
    cout << "numFiles " << numFiles << endl;
    
    if(numFiles == 0)
	cout << "The number of files is 0!" << endl;

    // TODO: if one row is too long, 
    // i.e. the shape of decomposition is long, and thin in one direction, 
    // alternative decomposition scheme may be more efficient.

    // row-wise decomposition

    local_grid_size_x = GRID_SIZE_X;
    local_grid_size_y = (int)ceil((double)GRID_SIZE_Y/numFiles);


    // define overlap.. a funtion of (radius_sqr)
    // overlap size: sqrt(radius_sqr)/GRID_DIST_Y;

    // construct a map indicating which file corresponds which area
    if((gridMap = new GridMap*[numFiles]) == NULL)
	cout << "OutCoreInterp::OutCoreInterp() GridMap[] allocation error" << endl;

    for(i = 0; i < numFiles; i++)
        {
            int lower_bound = i * local_grid_size_y;
            int upper_bound = (i+1) * local_grid_size_y - 1;

            if(upper_bound >= GRID_SIZE_Y)
                upper_bound = GRID_SIZE_Y - 1;
	    
            int overlap_lower_bound = lower_bound - overlapSize;
            if(overlap_lower_bound < 0)
                overlap_lower_bound = 0;

            int overlap_upper_bound = upper_bound + overlapSize + 1;
            if(overlap_upper_bound >= GRID_SIZE_Y)
                overlap_upper_bound = GRID_SIZE_Y - 1;

            char fname[1024];

            struct timeval tp;
            gettimeofday(&tp, NULL);

            snprintf(fname, sizeof(fname), "data_%d_%ld", i, tp.tv_usec);

            gridMap[i] = new GridMap(i,
                                     GRID_SIZE_X,
                                     lower_bound,
                                     upper_bound,
                                     overlap_lower_bound,
                                     overlap_upper_bound,
                                     false,
                                     fname);
            if(gridMap[i] == NULL)
                cout << "OutCoreInterp::OutCoreInterp() GridMap alloc error" << endl;

            cout << "[" <<  lower_bound << "," << upper_bound << "]" << endl;
            cout << "[" << overlap_lower_bound << "," << overlap_upper_bound << "]" << endl;
        }

    // initializing queues
    qlist = new list<UpdateInfo> [numFiles];
    if(qlist == NULL)
	cout << "OutCoreInterp::OutCoreInterp() qlist alloc error" << endl;

    openFile = -1;

    //cout << overlapSize << endl;
    //cout << "data size: " << (size_x * size_y) << " numFiles: " << numFiles << " local_grid_size_y: " << local_grid_size_y << " overlap: " << overlapSize << endl; 
    //cout << fname << endl;
    //cout << "GridMap is created: " << i << endl;
}

OutCoreInterp::~OutCoreInterp()
{
    /*
      if(qlist != NULL)
      delete qlist;
    */

    for(int i = 0; i < numFiles; i++)
        {
            //cout << "gridMap " << i << " deleted" << endl;
            if(gridMap[i] != NULL)
                delete gridMap[i];
        }
    if(gridMap != NULL)
	delete gridMap;
}

int OutCoreInterp::init()
{
    // open up a memory mapped file
    openFile = 0;
    gridMap[openFile]->getGridFile()->map();

    return 0;
}

int OutCoreInterp::update(double data_x, double data_y, double data_z)
{
    // update()
    //	push the point info into an appropriate queue
    //	if the length of the queue exceeds a limit,
    //	then
    //		if another file is loaded, write back (lazy update)
    //		if the file we need has not opened,
    //			read a file from disk
    //		update the file (PROC1)

    // 
    // find which file should be updated

    /*
      if(data_x < 0){
      cout << "4. !!!! " << data_x << endl;
      return -1;
      }
    */

    int fileNum;

    //fileNum = upper_grid_y / local_grid_size_y;
    if((fileNum = findFileNum(data_y)) < 0)
        {
            cout << "OutCoreInterp::update() findFileNum() error!" << endl;
            cout << "data_x: " << data_x << " data_y: " << data_y << " grid_y: " << (int)(data_y/GRID_DIST_Y) << " fileNum: " << fileNum << " open file: " << openFile << endl;
            return -1;
        }

    if(openFile == fileNum)
        {
            // write into memory;
            updateInterpArray(fileNum, data_x, data_y, data_z);

        }else{
	UpdateInfo ui(data_x, data_y, data_z);
	qlist[fileNum].push_back(ui);

	if(qlist[fileNum].size() == QUEUE_LIMIT)
	    {
		//cout << "erase" << endl;
		if(openFile != -1)
		    {
			// if we use mmap and write directly into disk, no need this step.
			// munmap would be enough for this step..

			// write back to disk
			gridMap[openFile]->getGridFile()->unmap();
			openFile = -1;
		    }
		// upload from disk to memory
		gridMap[fileNum]->getGridFile()->map();
		openFile = fileNum;

		// pop every update information
		list<UpdateInfo>::const_iterator iter;

		for(iter = qlist[openFile].begin(); iter!= qlist[openFile].end(); iter++){ 
		    updateInterpArray(openFile, (*iter).data_x, (*iter).data_y, (*iter).data_z);
		}
		// flush
		qlist[openFile].erase(qlist[openFile].begin(), qlist[openFile].end());
	    }
    }

    return 0;
}

int OutCoreInterp::finish(char *outputName, int outputFormat, unsigned int outputType)
{
    int i, j;
    GridPoint *p;
    GridFile *gf;
    int len_y;
    int offset;

    //struct tms tbuf;
    clock_t t0, t1;


    /*
    // managing overlap.
    read adjacent blocks and store ghost cells and update the cells

    merging multiple files
    */
    if(openFile != -1){
	gridMap[openFile]->getGridFile()->unmap();
	openFile = -1;
    }

    ////////////////////////////////////////////////////////////
    // flushing
    // can be moved inside the communications
    for(i = 0; i < numFiles; i++)
        {
            if(qlist[i].size() != 0)
                {
                    if((gf = gridMap[i]->getGridFile()) == NULL)
                        {
                            cout << "OutCoreInterp::finish() getGridFile() NULL" << endl;
                            return -1;
                        }

                    gf->map();
                    openFile = i;

                    // flush UpdateInfo queue
                    // pop every update information
                    list<UpdateInfo>::const_iterator iter;

                    for(iter = qlist[i].begin(); iter!= qlist[i].end(); iter++){ 
                        updateInterpArray(i, (*iter).data_x, (*iter).data_y, (*iter).data_z);
                    }
                    qlist[i].erase(qlist[i].begin(), qlist[i].end());

                    gf->unmap();
                    openFile = -1;
                    gf = NULL;
                }
        }
    ////////////////////////////////////////////////////////////

    for(i = 0; i < numFiles - 1; i++)
        {
            //////////////////////////////////////////////////////////////
            // read the upside overlap

            if((gf = gridMap[i]->getGridFile()) == NULL)
                {
                    cout << "OutCoreInterp::finish() getGridFile() NULL" << endl;
                    return -1;
                }

            if(gf->map() != -1)
                {
                    openFile = i;
		    // Sriram's edit to copy over DEM values to overlap also
                    len_y = 2 * (gridMap[i]->getOverlapUpperBound() - gridMap[i]->getUpperBound() - 1);

                    if((p = (GridPoint *)malloc(sizeof(GridPoint) * len_y * GRID_SIZE_X)) == NULL)
                        {
                            cout << "OutCoreInterp::finish() malloc error" << endl;
                            return -1;
                        }

                    int start = (gridMap[i]->getOverlapUpperBound() - gridMap[i]->getOverlapLowerBound() - len_y) * GRID_SIZE_X;
                    cout << "copy from " << start << " to " << (start + len_y * GRID_SIZE_X) << endl;

                    memcpy(p, &(gf->interp[start]), sizeof(GridPoint) * len_y * (GRID_SIZE_X) );

                    gf->unmap();
                    gf = NULL;
                    openFile = -1;

                } else {
		cout << "OutCoreInterp::finish() gf->map() error" << endl;
		return -1;
	    }
            //////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////
            // update (i+1)th component
            if((gf = gridMap[i+1]->getGridFile()) == NULL)
                {
                    cout << "OutCoreInterp::finish() getGridFile() NULL" << endl;
                    return -1;
                }

            if(gf->map() != -1)
                {
		    offset = 0;
                    openFile = i - 1;

                    for(j = 0; j < len_y * GRID_SIZE_X; j++)
                        {
                            if(gf->interp[j + offset].Zmin > p[j].Zmin)
                                gf->interp[j + offset].Zmin = p[j].Zmin;

                            if(gf->interp[j + offset].Zmax < p[j].Zmax)
                                gf->interp[j + offset].Zmax = p[j].Zmax;

                            gf->interp[j + offset].Zmean += p[j].Zmean;
                            gf->interp[j + offset].count += p[j].count;
			    
			    if (p[j].sum != -1) {
				gf->interp[j + offset].Zidw += p[j].Zidw;
				gf->interp[j + offset].sum += p[j].sum;
			    } else {
				gf->interp[j + offset].Zidw = p[j].Zidw;
				gf->interp[j + offset].sum = p[j].sum;
			    }
                        }

                    if(p != NULL){
                        free(p);
                        p = NULL;
                    }

                    gf->unmap();
                    gf = NULL;
                    openFile = -1;

                } else {
		cout << "OutCoreInterp::finish() gf->map() error" << endl;
		return -1;
	    }
            //////////////////////////////////////////////////////////////
        }

    for(i = numFiles -1; i > 0; i--)
        {
            //////////////////////////////////////////////////////////////
            // read the downside overlap

            if((gf = gridMap[i]->getGridFile()) == NULL)
                {
                    cout << "GridFile is NULL" << endl;
                    return -1;
                }

            if(gf->map() != -1)
                {
                    openFile = i;
                    len_y = 2 * (gridMap[i]->getLowerBound() - gridMap[i]->getOverlapLowerBound());

                    if((p = (GridPoint *)malloc(sizeof(GridPoint) * len_y * GRID_SIZE_X)) == NULL)
                        {
                            cout << "OutCoreInterp::finish() malloc error" << endl;
                            return -1;
                        }

                    memcpy(p, &(gf->interp[0]), len_y * sizeof(GridPoint) * GRID_SIZE_X);

                    //finalize();
	
                    gf->unmap();
                    gf = NULL;
                    openFile = -1;
                } else {
		cout << "OutCoreInterp::finish gf->map() error" << endl;
		return -1;
	    }
            //////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////
            // update (i-1)th component
            if((gf = gridMap[i-1]->getGridFile()) == NULL)
                {
                    cout << "GridFile is NULL" << endl;
                    return -1;
                }

            if(gf->map() != -1)
                {
                    offset = (gridMap[i-1]->getOverlapUpperBound() - gridMap[i-1]->getOverlapLowerBound() - len_y) * GRID_SIZE_X;
                    openFile = i - 1;

                    for(j = 0; j < len_y * GRID_SIZE_X; j++)
                        {
			    // Sriram - the overlap already contains the correct values
			    gf->interp[j + offset].Zmin = p[j].Zmin;
			    gf->interp[j + offset].Zmax = p[j].Zmax;
                            gf->interp[j + offset].Zmean = p[j].Zmean;
                            gf->interp[j + offset].count = p[j].count;
                            gf->interp[j + offset].Zidw = p[j].Zidw;
                            gf->interp[j + offset].sum = p[j].sum;
                        }

                    //if(i - 1 == 0)
                    //finalize();

                    if(p != NULL){
                        free(p);
                        p = NULL;
                    }

                    gf->unmap();
                    gf = NULL;
                    openFile = -1;
                }
            //////////////////////////////////////////////////////////////
        }

    for(i = 0; i < numFiles; i++)
        {
            gridMap[i]->getGridFile()->map();
            openFile = i;
            finalize();
            gridMap[i]->getGridFile()->unmap();
            openFile = -1;
        }



    t0 = clock();
    //t0 = times(&tbuf);

    // merge pieces into one file
    if(outputFile(outputName, outputFormat, outputType) < 0)
        {
            cout << "OutCoreInterp::finish outputFile error" << endl;
            return -1;
        }

    t1 = clock();
    //t1 = times(&tbuf);
    printf("Output Execution time: %10.2f\n", (double)(t1 - t0)/CLOCKS_PER_SEC);

    return 0;
}

void OutCoreInterp::updateInterpArray(int fileNum, double data_x, double data_y, double data_z)
{
    double x;
    double y;

    int lower_grid_x;
    int lower_grid_y;

    lower_grid_x = (int)floor((double)data_x/GRID_DIST_X); // global/local coordinate
    lower_grid_y = (int)floor((double)data_y/GRID_DIST_Y); // global coordinate

    x = data_x - lower_grid_x * GRID_DIST_X;
    y = data_y - lower_grid_y * GRID_DIST_Y;


    //if(lower_grid_y == 30 && data_y > GRID_DIST_Y * lower_grid_y)
    //printf("(%f %f) = (%d, %d)\n", data_x, data_y, lower_grid_x, lower_grid_y);

    // if the opened file is not the 0th file, then you have to consider the offset of the grid of the file!!!
    lower_grid_y -= gridMap[fileNum]->getOverlapLowerBound(); // local coordinate

    //cout << fileNum << ":(" << lower_grid_x << "," << lower_grid_y << ")" << endl;

    update_first_quadrant(fileNum, data_z, lower_grid_x + 1, lower_grid_y + 1, GRID_DIST_X -x, GRID_DIST_Y - y);
    update_second_quadrant(fileNum, data_z, lower_grid_x, lower_grid_y + 1, x, GRID_DIST_Y - y);
    update_third_quadrant(fileNum, data_z, lower_grid_x, lower_grid_y, x, y);
    update_fourth_quadrant(fileNum, data_z, lower_grid_x + 1, lower_grid_y, GRID_DIST_X - x, y);
}


void OutCoreInterp::update_first_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y)
{
    // base_x, base_y: local coordinates

    int i;
    int j;
    int ub = gridMap[fileNum]->getOverlapUpperBound() - gridMap[fileNum]->getOverlapLowerBound();
    //int ub = gridMap[fileNum]->getOverlapUpperBound();
    //cout << "fileNum: " << fileNum << " ub: " << ub << endl;
    //double temp;

    //printf("radius: %f ", radius_sqrt);

    for(i = base_x; i < GRID_SIZE_X; i++)
        {
            for(j = base_y; j < ub; j++)
                {
                    // i, j, base_x, base_y: local coordinates
                    double distance = 	((i - base_x)*GRID_DIST_X + x) * ((i - base_x)*GRID_DIST_X + x) + 
                        ((j - base_y)*GRID_DIST_Y + y) * ((j - base_y)*GRID_DIST_Y + y) ;

                    if(distance <= radius_sqr)
                        {
                            // update GridPoint
                            updateGridPoint(fileNum, i, j, data_z, distance);

                        } else if(j == base_y) {
			//printf("return ");
			return;
		    } else {
			//printf("break ");
			break;
		    }
                }
        }
}

	
void OutCoreInterp::update_second_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y)
{
    int i;
    int j;
    int ub = gridMap[fileNum]->getOverlapUpperBound() - gridMap[fileNum]->getOverlapLowerBound();
    //int ub = gridMap[fileNum]->getOverlapUpperBound();

    for(i = base_x; i >= 0; i--)
        {
            for(j = base_y; j < ub; j++)
                {
                    double distance = 	((base_x - i)*GRID_DIST_X + x) * ((base_x - i)*GRID_DIST_X + x) + 
                        ((j - base_y)*GRID_DIST_Y + y) * ((j - base_y)*GRID_DIST_Y + y);

                    if(distance <= radius_sqr)
                        {
                            //printf("(%d %d) ", i, j);
                            //interp[i][j]++;

                            updateGridPoint(fileNum, i, j, data_z, sqrt(distance));

                        } else if(j == base_y){
			return;
		    } else {
			break;
		    }
                }
        }

}

	
void OutCoreInterp::update_third_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y)
{
    int i;
    int j;
    //int lb = gridMap[fileNum]->getOverlapLowerBound();

    for(i = base_x; i >= 0; i--)
        {
            //for(j = base_y; j >= lb; j--)
            for(j = base_y; j >= 0; j--)
                {
                    double distance = 	((base_x - i)*GRID_DIST_X + x) * ((base_x - i)*GRID_DIST_X + x) + 
                        ((base_y - j)*GRID_DIST_Y + y) * ((base_y - j)*GRID_DIST_Y + y);

                    if(distance <= radius_sqr)
                        {
                            updateGridPoint(fileNum, i, j, data_z, sqrt(distance));
                        } else if(j == base_y){
			return;
		    } else {
			break;
		    }
                }
        }
}

void OutCoreInterp::update_fourth_quadrant(int fileNum, double data_z, int base_x, int base_y, double x, double y)
{
    int i, j;
    //int lb = gridMap[fileNum]->getOverlapLowerBound();

    for(i = base_x; i < GRID_SIZE_X; i++)
        {
            //for(j = base_y; j >= lb; j--)
            for(j = base_y; j >= 0; j--)
                {
                    double distance = 	((i - base_x)*GRID_DIST_X + x) * ((i - base_x)*GRID_DIST_X + x) + 
                        ((base_y - j)*GRID_DIST_Y + y) * ((base_y - j)*GRID_DIST_Y + y);

                    if(distance <= radius_sqr)
                        {
                            //printf("(%d %d) ", i, j);
                            //interp[i][j]++;
                            updateGridPoint(fileNum, i, j, data_z, sqrt(distance));
                        } else if (j == base_y) {
			return ;
		    } else {
			break;
		    }
                }
        }
}

void OutCoreInterp::updateGridPoint(int fileNum, int x, int y, double data_z, double distance)
{
    unsigned int coord = y * local_grid_size_x + x;
    GridFile *gf = gridMap[fileNum]->getGridFile();

    if(gf == NULL || gf->interp == NULL)
        {
            //cout << "OutCoreInterp::updateGridPoint() gridFile is NULL" << endl;
            return;
        }

    if(coord < gf->getMemSize() && coord >= 0)
        {
            if(gf->interp[coord].Zmin > data_z)
                gf->interp[coord].Zmin = data_z;
            if(gf->interp[coord].Zmax < data_z)
                gf->interp[coord].Zmax = data_z;

            gf->interp[coord].Zmean += data_z;
            gf->interp[coord].count++;

	    double dist = pow(distance, Interpolation::WEIGHTER);
	    if (gf->interp[coord].sum != -1) {
		if (dist != 0) {
		    gf->interp[coord].Zidw += data_z/dist;
		    gf->interp[coord].sum += 1/dist;
		} else {
		    gf->interp[coord].Zidw = data_z;
		    gf->interp[coord].sum = -1;
		}
	    } else {
		// do nothing
	    }
        } else {
	cout << "OutCoreInterp::updateGridPoint() Memory Access Violation!" << endl;
    }
}

int OutCoreInterp::outputFile(char *outputName, int outputFormat, unsigned int outputType)
{
    int i, j, k, l;

    FILE **arcFiles;
    char arcFileName[1024];

    FILE **gridFiles;
    char gridFileName[1024];

    char *ext[5] = {".min", ".max", ".mean", ".idw", ".den"};
    unsigned int type[5] = {OUTPUT_TYPE_MIN, OUTPUT_TYPE_MAX, OUTPUT_TYPE_MEAN, OUTPUT_TYPE_IDW, OUTPUT_TYPE_DEN};
    int numTypes = 5;


    // open ArcGIS files
    if(outputFormat == OUTPUT_FORMAT_ARC_ASCII || outputFormat == OUTPUT_FORMAT_ALL)
        {
            if((arcFiles = (FILE **)malloc(sizeof(FILE *) *  numTypes)) == NULL)
                {
                    cout << "Arc File open error: " << endl;
                    return -1;
                }

            for(i = 0; i < numTypes; i++)
                {
                    if(outputType & type[i])
                        {
                            strncpy(arcFileName, outputName, sizeof(arcFileName));
                            strncat(arcFileName, ext[i], strlen(ext[i]));
                            strncat(arcFileName, ".asc", strlen(".asc"));

                            if((arcFiles[i] = fopen64(arcFileName, "w+")) == NULL)
                                {
                                    cout << "File open error: " << arcFileName << endl;
                                    return -1;
                                }
                        }else{
			arcFiles[i] = NULL;
		    }
                }
        } else {
	arcFiles = NULL;
    }

    // open Grid ASCII files
    if(outputFormat == OUTPUT_FORMAT_GRID_ASCII || outputFormat == OUTPUT_FORMAT_ALL)
        {
            if((gridFiles = (FILE **)malloc(sizeof(FILE *) * numTypes)) == NULL)
                {
                    cout << "File array allocation error" << endl;
                    return -1;
                }

            for(i = 0; i < numTypes; i++)
                {
                    if(outputType & type[i])
                        {
                            strncpy(gridFileName, outputName, sizeof(arcFileName));
                            strncat(gridFileName, ext[i], strlen(ext[i]));
                            strncat(gridFileName, ".grid", strlen(".grid"));

                            if((gridFiles[i] = fopen64(gridFileName, "w+")) == NULL)
                                {
                                    cout << "File open error: " << gridFileName << endl;
                                    return -1;
                                }
                        }else{
			gridFiles[i] = NULL;
		    }
                }
        } else {
	gridFiles = NULL;
    }

    // print ArcGIS headers
    if(arcFiles != NULL)
        {
            for(i = 0; i < numTypes; i++)
                {
                    if(arcFiles[i] != NULL)
                        {
                            fprintf(arcFiles[i], "ncols %d\n", GRID_SIZE_X);
                            fprintf(arcFiles[i], "nrows %d\n", GRID_SIZE_Y);
                            fprintf(arcFiles[i], "xllcorner %f\n", min_x);
                            fprintf(arcFiles[i], "yllcorner %f\n", min_y);
                            fprintf(arcFiles[i], "cellsize %f\n", GRID_DIST_X);
                            fprintf(arcFiles[i], "NODATA_value -9999\n");
                        }
                }
        }

    // print Grid headers
    if(gridFiles != NULL)
        {
            for(i = 0; i < numTypes; i++)
                {
                    if(gridFiles[i] != NULL)
                        {
                            fprintf(gridFiles[i], "north: %f\n", max_y);
                            fprintf(gridFiles[i], "south: %f\n", min_y);
                            fprintf(gridFiles[i], "east: %f\n", max_x);
                            fprintf(gridFiles[i], "west: %f\n", min_x);
                            fprintf(gridFiles[i], "rows: %d\n", GRID_SIZE_Y);
                            fprintf(gridFiles[i], "cols: %d\n", GRID_SIZE_X);
                        }
                }
        }


    for(i = numFiles -1; i >= 0; i--)
        {
            GridFile *gf = gridMap[i]->getGridFile();
            gf->map();

            int start = gridMap[i]->getLowerBound() - gridMap[i]->getOverlapLowerBound();
            int end = gridMap[i]->getUpperBound() - gridMap[i]->getOverlapLowerBound() + 1;

            //int start = (gridMap[i]->getLowerBound() - gridMap[i]->getOverlapLowerBound()) * GRID_SIZE_X;
            //int end = (gridMap[i]->getUpperBound() - gridMap[i]->getOverlapLowerBound() + 1) * GRID_SIZE_X;

            cout << "Merging " << i << ": from " << (start) << " to " << (end) << endl;
            cout << "        " << i << ": from " << (start/GRID_SIZE_X) << " to " << (end/GRID_SIZE_X) << endl;
	
            for(j = end - 1; j >= start; j--)
                {
                    for(k = 0; k < GRID_SIZE_X; k++)
                        {
                            int index = j * GRID_SIZE_X + k;

                            if(arcFiles != NULL)
                                {
                                    // Zmin
                                    if(arcFiles[0] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                //if(gf->interp[k][j].Zmin == 0)
                                                fprintf(arcFiles[0], "-9999 ");
                                            else
                                                //fprintf(arcFiles[0], "%f ", gf->interp[j][i].Zmin);
                                                fprintf(arcFiles[0], "%f ", gf->interp[index].Zmin);
                                        }

                                    // Zmax
                                    if(arcFiles[1] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(arcFiles[1], "-9999 ");
                                            else
                                                fprintf(arcFiles[1], "%f ", gf->interp[index].Zmax);
                                        }

                                    // Zmean
                                    if(arcFiles[2] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(arcFiles[2], "-9999 ");
                                            else
                                                fprintf(arcFiles[2], "%f ", gf->interp[index].Zmean);
                                        }

                                    // Zidw
                                    if(arcFiles[3] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(arcFiles[3], "-9999 ");
                                            else
                                                fprintf(arcFiles[3], "%f ", gf->interp[index].Zidw);
                                        }

                                    // count
                                    if(arcFiles[4] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(arcFiles[4], "-9999 ");
                                            else
                                                fprintf(arcFiles[4], "%d ", gf->interp[index].count);
                                        }
                                }

                            if(gridFiles != NULL)
                                {
                                    // Zmin
                                    if(gridFiles[0] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(gridFiles[0], "-9999 ");
                                            else
                                                fprintf(gridFiles[0], "%f ", gf->interp[index].Zmin);
                                        }

                                    // Zmax
                                    if(gridFiles[1] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(gridFiles[1], "-9999 ");
                                            else
                                                fprintf(gridFiles[1], "%f ", gf->interp[index].Zmax);
                                        }

                                    // Zmean
                                    if(gridFiles[2] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(gridFiles[2], "-9999 ");
                                            else
                                                fprintf(gridFiles[2], "%f ", gf->interp[index].Zmean);
                                        }

                                    // Zidw
                                    if(gridFiles[3] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(gridFiles[3], "-9999 ");
                                            else
                                                fprintf(gridFiles[3], "%f ", gf->interp[index].Zidw);
                                        }

                                    // count
                                    if(gridFiles[4] != NULL)
                                        {
					    if(gf->interp[index].empty == 0 &&
					       gf->interp[index].filled == 0)
                                                fprintf(gridFiles[4], "-9999 ");
                                            else
                                                fprintf(gridFiles[4], "%d ", gf->interp[index].count);
                                        }
                                }
                        }
	    	
                    if(arcFiles != NULL)
                        for(l = 0; l < numTypes; l++)
                            {
                                if(arcFiles[l] != NULL)
                                    fprintf(arcFiles[l], "\n");
                            }
	
                    if(gridFiles != NULL)
                        for(l = 0; l < numTypes; l++)
                            {
                                if(gridFiles[l] != NULL)
                                    fprintf(gridFiles[l], "\n");
                            }
                }

            gf->unmap();
        }

    // close files
    if(gridFiles != NULL)
        {
            for(i = 0; i < numTypes; i++)
                {
                    if(gridFiles[i] != NULL)
                        fclose(gridFiles[i]);
                }
        }

    if(arcFiles != NULL)
        {
            for(i = 0; i < numTypes; i++)
                {
                    if(arcFiles[i] != NULL)
                        fclose(arcFiles[i]);
                }
        }

    return 0;
}

int OutCoreInterp::findFileNum(double data_y)
{
    int i;

    for(i = 0; i < numFiles; i++)
        {
            if(data_y <= gridMap[i]->getUpperBound() * GRID_DIST_Y &&
               data_y >= gridMap[i]->getLowerBound() * GRID_DIST_Y)
                return i;
            else if(i < numFiles - 1 && 
                    data_y > gridMap[i]->getUpperBound() * GRID_DIST_Y &&
                    data_y < gridMap[i+1]->getLowerBound() * GRID_DIST_Y)
                return i;
            else if(i == numFiles - 1 && 
                    data_y >= gridMap[i]->getUpperBound() * GRID_DIST_Y)
                {
                    //cout << "here" << endl;
                    return i;
                }
        }

    cout << "findFileNum() error" << endl;

    return -1;
}

void OutCoreInterp::finalize()
{
    int i;
    int start;
    int end;
    int overlapEnd;
    GridFile *gf;

    if(openFile == -1)
        {
            cout << "OutCoreInterp::finalize() no open file" << endl;
            return;
        }

    start = (gridMap[openFile]->getLowerBound() - gridMap[openFile]->getOverlapLowerBound()) * GRID_SIZE_X;
    end = (gridMap[openFile]->getUpperBound() - gridMap[openFile]->getOverlapLowerBound() + 1) * GRID_SIZE_X;
    overlapEnd = (gridMap[openFile]->getOverlapUpperBound() - gridMap[openFile]->getOverlapLowerBound() + 1) * GRID_SIZE_X;
    gf = gridMap[openFile]->getGridFile();

    cout << openFile << ": from " << (start) << " to " << (end) << endl;
    cout << openFile << ": from " << (start/GRID_SIZE_X) << " to " << (end/GRID_SIZE_X) << endl;

    for(i = 0; i < overlapEnd; i++)
        {	    
            if(gf->interp[i].Zmin == DBL_MAX)
                gf->interp[i].Zmin = 0;

            if(gf->interp[i].Zmax == -DBL_MAX)
                gf->interp[i].Zmax = 0;

            if(gf->interp[i].count != 0) {
                gf->interp[i].Zmean /= gf->interp[i].count ;
                gf->interp[i].empty = 1;
            }
            else
                gf->interp[i].Zmean = 0 ;

            if(gf->interp[i].sum != 0 && gf->interp[i].sum != -1)
                gf->interp[i].Zidw /= gf->interp[i].sum;
            else if (gf->interp[i].sum == -1) {
		// do nothing
	    } else
                gf->interp[i].Zidw = 0;
        }

    // Sriram's edit: Fill zeros using the window size parameter
    if (window_size != 0) {
        int window_dist = window_size / 2;
        for (i = start; i < end; i++)
            {
                double new_sum=0.0;
                if (gf->interp[i].empty == 0) {
                    for (int p = 0; p < window_size; p++) {
                        for (int q = 0; q < window_size; q++) {
                            // make sure that this is not an edge
                            int column = i - (i/local_grid_size_x)*local_grid_size_x;
                            if (((column + (p-window_dist)) < 0) ||
                                (column + (p-window_dist) >= local_grid_size_x)) {
                                continue;
                            }

                            int neighbor = (q-window_dist)*local_grid_size_x + (p-window_dist) + i;
			    if (neighbor == i)
				continue;

                            if ((neighbor >= 0) && (neighbor <= overlapEnd))
                                if (gf->interp[neighbor].empty != 0) {

				    double distance = max(fabs(p-window_dist), fabs(q-window_dist));
				    gf->interp[i].Zmean += gf->interp[neighbor].Zmean/(pow(distance,Interpolation::WEIGHTER));
				    gf->interp[i].Zidw += gf->interp[neighbor].Zidw/(pow(distance,Interpolation::WEIGHTER));
				    gf->interp[i].Zmin += gf->interp[neighbor].Zmin/(pow(distance,Interpolation::WEIGHTER));
				    gf->interp[i].Zmax += gf->interp[neighbor].Zmax/(pow(distance,Interpolation::WEIGHTER));
				    new_sum += 1/(pow(distance,Interpolation::WEIGHTER));
                                }
			}
		    }
		}
		if (new_sum > 0) {
		    gf->interp[i].Zmean /= new_sum;
		    gf->interp[i].Zidw /= new_sum;
		    gf->interp[i].Zmin /= new_sum;
		    gf->interp[i].Zmax /= new_sum;
		    gf->interp[i].filled = 1;
		}
            }
    }
}

/*
  cout << endl << "buffer p size: " << sizeof(GridPoint) * len_y * GRID_SIZE_X << endl;
  cout << "start: " << start << " len_y: " << len_y << endl;

  cout << "olb: " << gridMap[i]->getOverlapLowerBound() << " lb: " << gridMap[i]->getLowerBound() << endl;
  cout << "oub: " << gridMap[i]->getOverlapUpperBound() << " ub: " << gridMap[i]->getUpperBound() << endl;
  cout << "total size: " << gridMap[i]->getOverlapUpperBound() * GRID_SIZE_X + GRID_SIZE_X << endl;

  cout << "memcpy size: " << sizeof(GridPoint) * len_y * (GRID_SIZE_X - 45) << endl;
  cout << "mmap size: " << gridMap[i]->getGridFile()->getMemSize() << endl;
  cout << "start addr: " << start * sizeof(GridPoint) << endl << endl;

*/
