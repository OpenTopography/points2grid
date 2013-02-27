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
* LIDAR DEM Generation
* This program takes a set of randomly distributed points (ASCII or LAS) 
* as input, and generates gridded points (DEM)
* Based on the notes by Prof. Ramon Arrowsmith(ramon.arrowsmith@asu.edu)
* Authors: Han S Kim (hskim@cs.ucsd.edu), Sriram Krishnan (sriram@sdsc.edu)
*
*/

/*

 *
 * FileName: main.cpp
 * Author: Han S Kim (hskim@cs.ucsd.edu), Sriram Krishnan (sriram@sdsc.edu)
 * Description: this program starts from this file
 *
 */
 
#include "Interpolation.h"
#include "Global.h"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/times.h>
#include <curl/curl.h>
#include <string.h>

void printUsage();


int main(int argc, char **argv)
{
  //struct tms tbuf;
  clock_t t0, t1;

  // parameters
  char inputName[1024] = {0};
  char inputURL[2048] = {0};
  char outputName[1024] = {0};
  int input_format = INPUT_ASCII;
  int output_format = 0;
  unsigned int type = 0x00000000;
  double GRID_DIST_X = 6.0;
  double GRID_DIST_Y = 6.0;
  double searchRadius = (double) sqrt(2.0) * GRID_DIST_X;
  int window_size = 0;

  if(argc < 5)
    {
      printUsage();
      exit(0);
    }

  // argument processing..
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"output_format", 1, 0, 0},
	  {"resolution", 1, 0, 0},
	  {"resolution-x", 1, 0, 0},
	  {"resolution-y", 1, 0, 0},
	  {"min", 0, 0, 0},
	  {"max", 0, 0, 0},
	  {"mean", 0, 0, 0},
	  {"idw", 0, 0, 0},
	  {"den", 0, 0, 0},
	  {"all", 0, 0, 0},
	  {"fill", 0, 0, 0},
	  {"fill_window_size", 1, 0, 0},
	  {"input_format", 1, 0, 0},
	  {0, 0, 0, 0}
	};

      int option_index = 0;

      int c = getopt_long(argc, argv, "i:o:r:l:", long_options, &option_index);

      if(c == -1)
	break;

      switch(c)
	{
	case 0:

	  switch(option_index)
	    {
	    case 0:
	      if(!strncmp(optarg, "all", strlen("all")))
		output_format = OUTPUT_FORMAT_ALL;
	      else if(!strncmp(optarg, "arc", strlen("arc")))
		output_format = OUTPUT_FORMAT_ARC_ASCII;
	      else if(!strncmp(optarg, "grid", strlen("grid")))
		output_format = OUTPUT_FORMAT_GRID_ASCII;
	      else{
		printUsage();
		exit(0);
	      }
	      break;

	    case 1:
	      if(optarg != NULL){
		GRID_DIST_X = atof(optarg);
		GRID_DIST_Y = atof(optarg);
		if(searchRadius == sqrt(2.0) * 6.0)
		  searchRadius = (double) sqrt(2.0) * GRID_DIST_X;

		if(GRID_DIST_X == 0){
		  printUsage();
		  exit(0);
		}
	      }else{
		printUsage();
		exit(0);
	      }

	    case 2:
	      if(optarg != NULL){
		GRID_DIST_X = atof(optarg);
		if(searchRadius == sqrt(2.0) * 6.0)
		  searchRadius = (double) sqrt(2.0) * GRID_DIST_X;

		if(GRID_DIST_X == 0){
		  printUsage();
		  exit(0);
		}
	      }else{
		printUsage();
		exit(0);
	      }
	      break;

	    case 3:
	      if(optarg != NULL){
		GRID_DIST_Y = atof(optarg);
		if(GRID_DIST_Y == 0){
		  printUsage();
		  exit(0);
		}
	      }else{
		printUsage();
		exit(0);
	      }
	      break;

	    case 4:
	      type |= OUTPUT_TYPE_MIN;
	      break;
	    case 5:
	      type |= OUTPUT_TYPE_MAX;
	      break;
	    case 6:
	      type |= OUTPUT_TYPE_MEAN;
	      break;
	    case 7:
	      type |= OUTPUT_TYPE_IDW;
	      break;
	    case 8:
	      type |= OUTPUT_TYPE_DEN;
	      break;
	    case 9:
	      type = OUTPUT_TYPE_ALL;
	      break;

	    case 10:
	      window_size = 3;
	      break;
	    case 11:
	      if(optarg != NULL){
		window_size = atoi(optarg);
		if(!((window_size == 3) || (window_size == 5) || (window_size == 7))) {
		  printUsage();
		  exit(0);
		}
	      }else{
		printUsage();
		exit(0);
	      }
	      break;
	    case 12:
	      if(!strncmp(optarg, "ascii", strlen("ascii")))
		input_format = INPUT_ASCII;
	      else if(!strncmp(optarg, "las", strlen("las")))
		input_format = INPUT_LAS;
	      else{
		printUsage();
		exit(0);
	      }
	      break;

	    default:
	      printUsage();
	      exit(0);
	    }

	  break;
	case 'i':

	  if(optarg != NULL)
	    strncpy(inputName, optarg, sizeof(inputName));
	  else{
	    printUsage();
	    exit(0);
	  }

	  break;
	case 'l':

	  if(optarg != NULL)
	    strncpy(inputURL, optarg, sizeof(inputURL));
	  else{
	    printUsage();
	    exit(0);
	  }

	  break;
	case 'o':
	  if(optarg != NULL)
	    strncpy(outputName, optarg, sizeof(inputName));
	  else{
	    printUsage();
	    exit(0);
	  }

	  break;
	case 'r':
	  if(optarg != NULL)
	    searchRadius = atof(optarg);

	  break;
	default:
	  printUsage();
	  exit(0);
	}
    }
  if(type == 0)
    type = OUTPUT_TYPE_ALL;

  if(((inputName == NULL || !strcmp(inputName, "")) && 
      (inputURL == NULL || !strcmp(inputURL, "")))
     || 
     (outputName == NULL || !strcmp(outputName, "")))
    {
      printUsage();
      exit(0);
    }

  // download file from URL, and set input name
  if (!((inputURL == NULL || !strcmp(inputURL, "")))) {
      CURL *curl;
      CURLcode res;

      /* get the file name from the URL */
      int i = 0;
      for(i = sizeof(inputURL); i>= 0; i--) { 
	  if(inputURL[i] == '/')
	      break;
      }
      strncpy(inputName, inputURL+i+1, sizeof(inputName));
 
      curl = curl_easy_init();
      if (!curl) {
	  cout << "Can't initialize curl object to download input from: " 
	       << inputURL << endl;
	  exit(1);
      }

      /* set URL */
      curl_easy_setopt(curl, CURLOPT_URL, inputURL);
      curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

      /* and write to file */
      FILE *fp;
      fp = fopen(inputName, "w");
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp); 

      /* perform the curl request and clean up */
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      fclose(fp);

      if (res != 0) {
	  cout << "Error while downloading input from: " << inputURL << endl;
	  exit(1);
      }

  }

  cout << "Parameters ************************" << endl;
  cout << "inputName: '" << inputName << "'" << endl;
  cout << "input_format: " << input_format << endl;
  cout << "outputName: '" << outputName << "'" << endl;
  cout << "GRID_DIST_X: " << GRID_DIST_X << endl;
  cout << "GRID_DIST_Y: " << GRID_DIST_Y << endl;
  cout << "searchRadius: " << searchRadius << endl;
  cout << "output_format: " << output_format << endl;
  cout << "type: " << type << endl;
  cout << "fill window size: " << window_size << endl;
  cout << "************************************" << endl;

  t0 = clock();
  //t0 = times(&tbuf);

  Interpolation *ip = new Interpolation(GRID_DIST_X, GRID_DIST_Y, searchRadius, window_size);

  if(ip->init(inputName, input_format) < 0)
    {
      fprintf(stderr, "Interpolation::init() error\n");
      return -1;
    }

  t1 = clock();
  //t1 = times(&tbuf);
  printf("Init + Min/Max time: %10.2f\n", (double)(t1 - t0)/CLOCKS_PER_SEC);

  t0 = clock();
  //t0 = times(&tbuf);

  if(ip->interpolation(inputName, outputName, input_format, output_format, type) < 0)
    {
      fprintf(stderr, "Interpolation::interpolation() error\n");
      return -1;
    }

  t1 = clock();
  //t1 = times(&tbuf);
  printf("Interpolation + Output time: %10.2f\n", (double)(t1 - t0)/CLOCKS_PER_SEC);


  //printf("i: %d\n", i);
  printf("# of data: %d\n", ip->getDataCount());
  //printf("Total Execution time: %10.2f\n", (t1 - t0)/(double) sysconf(_SC_CLK_TCK));
  printf("dimension: %d x %d\n", ip->getGridSizeX(), ip->getGridSizeY());

  delete ip;

  return 0;
}

void printUsage()
{
  cout << "*****************************************************" << endl;
  cout << "Usage: \n" << endl;
  cout << "% ./interp [-i <data_file_name> | -l <data_file_url>] -o <output_file_name> [--input_format=ascii|las] [--all] [--min] [--max] [--mean] [--idw] [--den] [--fill|--fill_window_size=<value>] [-r <search_radius>] [--output_format=all|arc|grid] [--resolution=<value>|--resolution-x=<value>|--resolution-y=<value>] "<< endl << endl;
  cout << "1. -i <data_file_name> | -l <data_file_url>:"<< endl;
  cout << "	- must be unzipped plain text file"<< endl << endl;
  cout << "2. -o <output_file_name>: "<< endl;
  cout << "	- without extension, i.e. if you want the output file to be test.asc, this parameter shoud be \"test\""<< endl << endl;
  cout << "3. Output Type: " << endl;
  cout << "	--min: the Zmin values are stored" << endl;
  cout << "	--max: the Zmax values are stored" << endl;
  cout << "	--mean: the Zmean values are stored" << endl;
  cout << "	--idw: the Zidw values are stored" << endl;
  cout << "	--den: the density values are stored" << endl;
  cout << "	--all (default): all the values are stored" << endl << endl;
  cout << "4. -r <search radius>"<< endl;
  cout << "	- specifies the search radius. The default value is square root 2 of horizontal distance in a grid cell"<< endl << endl;
  cout << "5. --input_format: "<< endl;
  cout << "     - 'ascii' expects input point cloud in ASCII format (default)"<< endl;
  cout << "     - 'las' expects input point cloud in LAS format"<< endl << endl;
  cout << "6. --output_format: "<< endl;
  cout << "	- 'all' generates every possible format, "<< endl;
  cout << "	- 'arc' for ArcGIS format, "<< endl;
  cout << "	- 'grid' for Ascii GRID format, "<< endl;
  cout << "	- the default value is --all"<< endl << endl;
  cout << "7. Resolution"<< endl;
  cout << "	- --resolution=<value>: The resolution is set to the specified value. Use square grids."<< endl;
  cout << "	- --resolution-x=<value> --resolution-y=<value>: Each parameter specifies each side of grid cells."<< endl;
  cout << "	- If not specified, default values (6ft) are used."<< endl << endl;
  cout << "8. Null Filling"<<endl;
  cout << "   --fill: fills nulls in the DEM. Default window size is 3."<<endl;
  cout << "   --fill_window_size=<value>: The fill window is set to value. Permissible values are 3, 5 and 7."<<endl;
  cout << "*****************************************************" << endl;
}

