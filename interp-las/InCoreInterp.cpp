#include "InCoreInterp.h"

#include <float.h>
#include <math.h>
#include "Interpolation.h"
#include "Global.h"
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <string.h>

InCoreInterp::InCoreInterp(double dist_x, double dist_y, 
			   int size_x, int size_y, 
			   double r_sqr, 
			   double _min_x, double _max_x, 
			   double _min_y, double _max_y,
			   int _window_size) 
{
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

    cout << "InCoreInterp created successfully" << endl;
}

InCoreInterp::~InCoreInterp()
{
    int i;
    for(i = 0; i < GRID_SIZE_X; i++)
	delete interp[i];
    delete interp;
}

int InCoreInterp::init()
{
    int i, j;

    interp = (GridPoint**)malloc(sizeof(GridPoint *) * GRID_SIZE_X);
    //interp = new GridPoint*[GRID_SIZE_X];
    if(interp == NULL)
	{
	    cout << "InCoreInterp::init() new allocate error" << endl;
	    return -1;
	}

    for(i = 0; i < GRID_SIZE_X; i++)
	{
	    interp[i] = (GridPoint *)malloc(sizeof(GridPoint) * GRID_SIZE_Y);
	    //interp[i] = new GridPoint[GRID_SIZE_Y];
	    if(interp[i] == NULL)
		{
		    cout << "InCoreInterp::init() new allocate error" << endl;
		    return -1;
		}
	}

    for(i = 0; i < GRID_SIZE_X; i++)
	for(j = 0; j < GRID_SIZE_Y; j++)
	    {
		interp[i][j].Zmin = DBL_MAX;
		interp[i][j].Zmax = -DBL_MAX;
		interp[i][j].Zmean = 0;
		interp[i][j].count = 0;
		interp[i][j].Zidw = 0;
		interp[i][j].sum = 0;
		interp[i][j].empty = 0;
		interp[i][j].filled = 0;
	    }

    cout << "InCoreInterp::init() done" << endl;

    return 0;
}

int InCoreInterp::update(double data_x, double data_y, double data_z)
{
    double x;
    double y;

    int lower_grid_x;	
    int lower_grid_y; 

    //cout << GRID_DIST_X << " " << GRID_DIST_Y;
    lower_grid_x = (int)floor((double)data_x/GRID_DIST_X);
    lower_grid_y = (int)floor((double)data_y/GRID_DIST_Y);

    if(lower_grid_x >= GRID_SIZE_X || lower_grid_y >= GRID_SIZE_Y)
	{
	    cout << "larger at (" << lower_grid_x << "," << lower_grid_y << "): ("<< data_x << ", " << data_y << ")" << endl;
	    return 0;
	}

    //printf("lower_grid_x: %d, grid_y: %d, arrX: %.2f, arrY: %.2f\n", lower_grid_x, lower_grid_y, arrX[i], arrY[i]);
    x = (data_x - (lower_grid_x) * GRID_DIST_X);
    y = (data_y - (lower_grid_y) * GRID_DIST_Y);

    //cout << "(" << data_x << " " << data_y << ")=(" << lower_grid_x << ", " << lower_grid_y << "): ";
    //printf("(%f %f) = (%d, %d): ", data_x, data_y, lower_grid_x, lower_grid_y);

    //if(lower_grid_y == 30 && data_y > GRID_DIST_Y * lower_grid_y)
    //printf("(%f %f) = (%d, %d)\n", data_x, data_y, lower_grid_x, lower_grid_y);

    update_first_quadrant(data_z, lower_grid_x+1, lower_grid_y+1, GRID_DIST_X - x, GRID_DIST_Y - y);
    update_second_quadrant(data_z, lower_grid_x, lower_grid_y+1, x, GRID_DIST_Y - y);
    update_third_quadrant(data_z, lower_grid_x, lower_grid_y, x, y);
    update_fourth_quadrant(data_z, lower_grid_x+1, lower_grid_y, GRID_DIST_X - x, y);

    //cout << "test" << endl;
    return 0;
}

int InCoreInterp::finish(char *outputName, int outputFormat, unsigned int outputType)
{
    int rc;
    int i,j;

    //struct tms tbuf;
    clock_t t0, t1;

    for(i = 0; i < GRID_SIZE_X; i++)
	for(j = 0; j < GRID_SIZE_Y; j++)
	    {
		if(interp[i][j].Zmin == DBL_MAX){
		    //		interp[i][j].Zmin = NAN;
		    interp[i][j].Zmin = 0;
		}

		if(interp[i][j].Zmax == -DBL_MAX){
		    //interp[i][j].Zmax = NAN;
		    interp[i][j].Zmax = 0;
		}

		if(interp[i][j].count != 0) {
		    interp[i][j].Zmean /= interp[i][j].count;
		    interp[i][j].empty = 1;
		} else{
		    //interp[i][j].Zmean = NAN;
		    interp[i][j].Zmean = 0;
		}

		if(interp[i][j].sum != 0 && interp[i][j].sum != -1)
		    interp[i][j].Zidw /= interp[i][j].sum;
		else if (interp[i][j].sum == -1){
		    // do nothing
		} else{
		    //interp[i][j].Zidw = NAN;
		    interp[i][j].Zidw = 0;
		}
	    }

    // Sriram's edit: Fill zeros using the window size parameter
    if (window_size != 0) {
	int window_dist = window_size / 2;
	for (int i = 0; i < GRID_SIZE_X; i++)
	    for (int j = 0; j < GRID_SIZE_Y; j++)
		{
		    if (interp[i][j].empty == 0) {
			double new_sum=0.0;
			for (int p = i - window_dist; p <= i + window_dist; p++) {
			    for (int q = j - window_dist; q <= j + window_dist; q++) {
				if ((p >= 0) && (p < GRID_SIZE_X) && (q >=0) && (q < GRID_SIZE_Y)) {
				    if ((p == i) && (q == j))
					continue;
		    
				    if (interp[p][q].empty != 0) {
					double distance = max(fabs(p-i), fabs(q-j));
					interp[i][j].Zmean += interp[p][q].Zmean/(pow(distance,Interpolation::WEIGHTER));
					interp[i][j].Zidw += interp[p][q].Zidw/(pow(distance,Interpolation::WEIGHTER));
					interp[i][j].Zmin += interp[p][q].Zmin/(pow(distance,Interpolation::WEIGHTER));
					interp[i][j].Zmax += interp[p][q].Zmax/(pow(distance,Interpolation::WEIGHTER));

					new_sum += 1/(pow(distance,Interpolation::WEIGHTER));
				    }
				}
			    }
			}
			if (new_sum > 0) {
			    interp[i][j].Zmean /= new_sum;
			    interp[i][j].Zidw /= new_sum;
			    interp[i][j].Zmin /= new_sum;
			    interp[i][j].Zmax /= new_sum;
			    interp[i][j].filled = 1;
			}
		    }
		}
    }

    t0 = clock();
    //t0 = times(&tbuf);

    if((rc = outputFile(outputName, outputFormat, outputType)) < 0)
	{
	    cout << "InCoreInterp::finish outputFile error" << endl;
	    return -1;
	}

    t1 = clock();
    //t1 = times(&tbuf);

    printf("Output Execution time: %10.2f\n", (double)(t1 - t0)/ CLOCKS_PER_SEC);


    return 0;
}

//////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////

void InCoreInterp::update_first_quadrant(double data_z, int base_x, int base_y, double x, double y)
{
    int i;
    int j;
    //double temp;

    //printf("radius: %f ", radius_sqrt);

    for(i = base_x; i < GRID_SIZE_X; i++)
	{
	    for(j = base_y; j < GRID_SIZE_Y; j++)
		{
		    /*
		      temp = (   ((i - base_x)*GRID_DIST + x) * ((i - base_x)*GRID_DIST + x) + 
		      ((j - base_y)*GRID_DIST + y) * ((j - base_y)*GRID_DIST + y)) ;
		      printf("%f ", temp);
		    */

		    double distance = 	((i - base_x)*GRID_DIST_X + x) * ((i - base_x)*GRID_DIST_X + x) + 
			((j - base_y)*GRID_DIST_Y + y) * ((j - base_y)*GRID_DIST_Y + y) ;

		    if(distance <= radius_sqr)
			{
			    //printf("(%d %d) ", i, j);
			    //interp[i][j]++;
		
			    // update GridPoint
			    updateGridPoint(i, j, data_z, distance);

			} else if(j == base_y) {
			//printf("return ");
			return;
		    } else {
			//printf("break ");
			break;
		    }
		}
	}

    //cout << "test2" << endl;
}

	
void InCoreInterp::update_second_quadrant(double data_z, int base_x, int base_y, double x, double y)
{
    int i;
    int j;

    for(i = base_x; i >= 0; i--)
	{
	    for(j = base_y; j < GRID_SIZE_Y; j++)
		{
		    double distance = 	((base_x - i)*GRID_DIST_X + x) * ((base_x - i)*GRID_DIST_X + x) + 
			((j - base_y)*GRID_DIST_Y + y) * ((j - base_y)*GRID_DIST_Y + y);

		    if(distance <= radius_sqr)
			{
			    //printf("(%d %d) ", i, j);
			    //interp[i][j]++;

			    updateGridPoint(i, j, data_z, sqrt(distance));

			} else if(j == base_y){
			return;
		    } else {
			break;
		    }
		}
	}

}

	
void InCoreInterp::update_third_quadrant(double data_z, int base_x, int base_y, double x, double y)
{
    int i;
    int j;

    for(i = base_x; i >= 0; i--)
	{
	    for(j = base_y; j >= 0; j--)
		{
		    double distance = 	((base_x - i)*GRID_DIST_X + x) * ((base_x - i)*GRID_DIST_X + x) + 
			((base_y - j)*GRID_DIST_Y + y) * ((base_y - j)*GRID_DIST_Y + y);

		    if(distance <= radius_sqr)
			{
			    //if(j == 30)
			    //printf("(%d %d)\n", i, j);
			    //interp[i][j]++;
			    updateGridPoint(i, j, data_z, sqrt(distance));
			} else if(j == base_y){
			return;
		    } else {
			break;
		    }
		}
	}
}

void InCoreInterp::update_fourth_quadrant(double data_z, int base_x, int base_y, double x, double y)
{
    int i, j;

    for(i = base_x; i < GRID_SIZE_X; i++)
	{
	    for(j = base_y; j >= 0; j--)
		{
		    double distance = 	((i - base_x)*GRID_DIST_X + x) * ((i - base_x)*GRID_DIST_X + x) + 
			((base_y - j)*GRID_DIST_Y + y) * ((base_y - j)*GRID_DIST_Y + y);

		    if(distance <= radius_sqr)
			{
			    //printf("(%d %d) ", i, j);
			    //interp[i][j]++;
			    updateGridPoint(i, j, data_z, sqrt(distance));
			} else if (j == base_y) {
			return ;
		    } else {
			break;
		    }
		}
	}
}

void InCoreInterp::updateGridPoint(int x, int y, double data_z, double distance)
{
#if 1 
	//improved sanity-check is called on every update of the grid.
        if(x >= GRID_SIZE_X || y >= GRID_SIZE_Y)
        {
                printf("skipping grid point update for %d, %d:\tz: %f, dist: %f\n", x,y,data_z, distance);
                return;
        }
#endif

    if(interp[x][y].Zmin > data_z)
	interp[x][y].Zmin = data_z;
    if(interp[x][y].Zmax < data_z)
	interp[x][y].Zmax = data_z;

    interp[x][y].Zmean += data_z;
    interp[x][y].count++;

    double dist = pow(distance, Interpolation::WEIGHTER);

    if(interp[x][y].sum != -1) {
	if(dist != 0) {
	    interp[x][y].Zidw += data_z/dist;
	    interp[x][y].sum += 1/dist;
	} else {
	    interp[x][y].Zidw = data_z;
	    interp[x][y].sum = -1;
	}
    } else {
	// do nothing
    }
}

void InCoreInterp::printArray()
{
    int i, j;

    for(i = 0; i < GRID_SIZE_X; i++)
	{
	    for(j = 1; j < GRID_SIZE_Y; j++)
		{
		    cout << interp[i][j].Zmin << ", " << interp[i][j].Zmax << ", ";
		    cout << interp[i][j].Zmean << ", " << interp[i][j].Zidw << endl;
		    //printf("%.20f ", interp[i][j].Zmax);
		}
	    //printf("\n");
	}
    cout << endl;
}

int InCoreInterp::outputFile(char *outputName, int outputFormat, unsigned int outputType)
{
    int i,j,k;

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


    //struct tms tbuf;
    //clock_t t0, t1;

    //t0 = times(&tbuf);

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

    // print data
    for(i = GRID_SIZE_Y - 1; i >= 0; i--)
	{
	    for(j = 0; j < GRID_SIZE_X; j++)
		{
		    if(arcFiles != NULL)
			{
			    // Zmin
			    if(arcFiles[0] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(arcFiles[0], "-9999 ");
				    else
					fprintf(arcFiles[0], "%f ", interp[j][i].Zmin);
				}

			    // Zmax
			    if(arcFiles[1] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(arcFiles[1], "-9999 ");
				    else
					fprintf(arcFiles[1], "%f ", interp[j][i].Zmax);
				}

			    // Zmean
			    if(arcFiles[2] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(arcFiles[2], "-9999 ");
				    else
					fprintf(arcFiles[2], "%f ", interp[j][i].Zmean);
				}

			    // Zidw
			    if(arcFiles[3] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(arcFiles[3], "-9999 ");
				    else
					fprintf(arcFiles[3], "%f ", interp[j][i].Zidw);
				}

			    // count
			    if(arcFiles[4] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(arcFiles[4], "-9999 ");
				    else
					fprintf(arcFiles[4], "%d ", interp[j][i].count);
				}
			}

		    if(gridFiles != NULL)
			{
			    // Zmin
			    if(gridFiles[0] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(gridFiles[0], "-9999 ");
				    else
					fprintf(gridFiles[0], "%f ", interp[j][i].Zmin);
				}

			    // Zmax
			    if(gridFiles[1] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(gridFiles[1], "-9999 ");
				    else
					fprintf(gridFiles[1], "%f ", interp[j][i].Zmax);
				}

			    // Zmean
			    if(gridFiles[2] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(gridFiles[2], "-9999 ");
				    else
					fprintf(gridFiles[2], "%f ", interp[j][i].Zmean);
				}

			    // Zidw
			    if(gridFiles[3] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(gridFiles[3], "-9999 ");
				    else
					fprintf(gridFiles[3], "%f ", interp[j][i].Zidw);
				}

			    // count
			    if(gridFiles[4] != NULL)
				{
				    if(interp[j][i].empty == 0 && 
				       interp[j][i].filled == 0)
					fprintf(gridFiles[4], "-9999 ");
				    else
					fprintf(gridFiles[4], "%d ", interp[j][i].count);
				}
			}
		}
	    if(arcFiles != NULL)
		for(k = 0; k < numTypes; k++)
		    {
			if(arcFiles[k] != NULL)
			    fprintf(arcFiles[k], "\n");
		    }
	    if(gridFiles != NULL)
		for(k = 0; k < numTypes; k++)
		    {
			if(gridFiles[k] != NULL)
			    fprintf(gridFiles[k], "\n");
		    }
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

    //t1 = times(&tbuf);
    //printf("elapsed time: %10.10f\n", (t1 - t0)/(double) sysconf(_SC_CLK_TCK));

    return 0;
}

