[![NSF-0930731](https://img.shields.io/badge/NSF-0930731-blue.svg)](https://nsf.gov/awardsearch/showAward?AWD_ID=0930731) 

Points2Grid
------------

Proven through thousands of jobs run via the OpenTopography Facility
(http://www.opentopography.org), Points2Grid is a robust and scalable tool 
that generates Digital Elevation Models (DEM) using a local gridding
method. The local gridding algorithm computes grid cell elevation using a
circular neighbourhood defined around each grid cell based on a radius
provided by the user. This neighbourhood is referred to as a bin, while the
grid cell is referred to as a DEM node. Up to four values — minimum,
maximum, mean, or inverse distance weighted (IDW) mean — are computed for
points that fall within the bin. These values are then assigned to the
corresponding DEM node and used to represent the elevation variation over
the neighbourhood represented by the bin. If no points are found within a
given bin, the DEM node receives a value of null. The Points2Grid service
also provides a null filing option, which applies an inverse distance
weighted focal mean via a square moving window of 3, 5, or 7 pixels to fill
cells in the DEM that have null values.
The local gridding approach employed by
Points2Grid is most effective in situations where LIDAR shot densities
exceed the resolution of the grid to be produced from these data.
Please direct all feedback to info@opentopography.org 


INSTALLATION
------------
Please read the INSTALL file for details on the installation process.

USE
---
If your pre-requisites (curl and liblas) are installed on non-standard
location, please set your LD_LIBRARY_PATH accordingly. To run complete
usage information, please type "points2grid" from the command-line. The
command-line options are as follows:

% ./interp [-i <data_file_name> | -l <data_file_url>] -o
  <output_file_name> [--input_format=ascii|las] [--all] [--min] [--max]
  [--mean] [--idw] [--den] [--fill|--fill_window_size=<value>] [-r
  <search_radius>] [--output_format=all|arc|grid]
  [--resolution=<value>|--resolution-x=<value>|--resolution-y=<value>]

1. -i <data_file_name> | -l <data_file_url>:
	- must be unzipped plain text file

2. -o <output_file_name>: 
	- without extension, i.e. if you want the output file to be
	test.asc, this parameter shoud be "test"

3. Output Type: 
	--min: the Zmin values are stored
	--max: the Zmax values are stored
	--mean: the Zmean values are stored
	--idw: the Zidw values are stored
	--den: the density values are stored
	--all (default): all the values are stored

4. -r <search radius>
	- specifies the search radius. The default value is square root 2
	of horizontal distance in a grid cell 

5. --input_format: 
        - 'ascii' expects input point cloud in ASCII format (default)
        - 'las' expects input point cloud in LAS format

6. --output_format: 
	- 'all' generates every possible format, 
	- 'arc' for ArcGIS format, 
	- 'grid' for Ascii GRID format, 
	- the default value is --all

7. Resolution
	- --resolution=<value>: The resolution is set to the specified
	  value. Use square grids.
	- --resolution-x=<value> --resolution-y=<value>: Each parameter
	  specifies each side of grid cells. 
	- If not specified, default values (6ft) are used.

8. Null Filling
        --fill: fills nulls in the DEM. Default window size is 3.
        --fill_window_size=<value>: The fill window is set to value.
	  Permissible values are 3, 5 and 7.
