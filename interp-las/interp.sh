#!/bin/bash

export LD_LIBRARY_PATH=/users/u4/geon/Software/liblas-src-1.2.1/install/lib:/usr/local/apps/gcc-3.4.5/lib/:/users/u4/geon/Software/curl-7.20.0/install/lib

/users/u4/geon/Software/interp-las/interp $*

exitVal=$?

# clean up the input file since it can get huge
# this is a hack - since it can be called something other than points.txt
# rm -f points.txt

# only do this is job execution is successful
if [ $exitVal == 0 ]; then 

    # make a tarball from the outputs - ignore the stderr from ls
    echo "************************************"
    echo "Creating tarball from job outputs"
    tar cvf dems.tar `ls *.grid *.asc 2>/dev/null`
    /usr/bin/gzip -2 dems.tar
    # zip dems.zip `ls *.grid *.asc 2>/dev/null`

    # delete individual output files
    # rm `ls *.grid *.asc 2>/dev/null`
    rm -f points.las points.txt
else
    exit $exitVal
fi
