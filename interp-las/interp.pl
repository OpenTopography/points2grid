#!/usr/bin/perl

###################################################
# 0. Constant

$host = "rocks-109.sdsc.edu";

$localDataDir = '/state/partition1/data/';

$inputFile = $ARGV[0];
#print "input file: $inputFile\n";

($directory, $fileName) = $inputFile =~ m/(.*\/)(.*)$/;
#print "fileName: $fileName\n";
#print "directory: $directory\n";

$cmd = "cp $inputFile.pointData.tar.gz $localDataDir$fileName.pointData.tar.gz";
system($cmd);

$directory = $localDataDir;
$inputFile = "$localDataDir"."$fileName";

#print "directory: $directory\n";
#print "inputfile: $inputFile\n";

$paramsFile = $ARGV[1];
#$fileNameTMP = "$directory"."dump.".$fileName;
$interpLog = '/home/geon/.interp/interp.dump'; 
$fileNameTMP = "/home/geon/.interp/interp"."$fileName".".dump"; 

$dataBaseDir = '/export/downloads/kepler/';
#$binDir = '/home/hskim/research/summer06/interp/out-of-core/bin';
$binDir = '/state/partition1/grass/LNet/interp/bin';


$cmd = "cd $binDir";
system($cmd);
###################################################


###################################################
# 1. input file

# untar/unzip
$cmd = "cd $directory; tar zxvf $inputFile.pointData.tar.gz >> $fileNameTMP 2>&1";
# print "$cmd\n";
system($cmd);
$cmd = "rm -f $localDataDir$fileName.pointData.tar.gz";
system($cmd);
#$inputFile = "$localDataDir"."$fileName";

# select the first three columns
$cmd = "awk -F\",\" \'{print \$1 \",\" \$2 \",\" \$3}\' $inputFile.txt > $inputFile.txt_";
system($cmd);
# print "$cmd\n";
###################################################

###################################################
# 2. parameters
foreach $key1 (qw(min max mean idw den)) {
    foreach $key2 (qw(arc grid)) {
	$key = $key1.$key2;
	$$key = 0;
    }
}
$radius = '';
$bineres = '';


# open parameters file
open(INFILE, $paramsFile) or die "ERROR: parameter file open error\n";
while ($line = <INFILE>) {
    $line =~ s/\r/\n/s;
    chomp $line;
    ($key,$val) = split /=/,$line;
    $$key = $val;
}
close(INFILE);

#remove parameters file when finished
#$cmd = "rm -f $paramsFile";
#system($cmd);

# make parameters
$parameters = '';

if ($radius != '') {
    $parameters = "-r $radius";
}

if ($bineres != '') {
    $parameters = "$parameters --resolution=$bineres";
}

foreach $key1 (qw(min max mean idw den)) {
    foreach $key2 (qw(arc grid)) {
	$key = $key1.$key2;
	if($$key == 1)
	{
	    $parameters = "$parameters --$key";
	}
    }
}
# print "$parameters\n";

###################################################



###################################################
# Run interp

$cmd = "$binDir/interp.sh -i $inputFile.txt_ -o $binDir/$fileName $parameters >> $fileNameTMP 2>&1";
system($cmd);
# print "$cmd\n";

###################################################
# Output Products

#create product directories
$productDir = "$dataBaseDir"."$fileName";
$cmd = "mkdir $productDir >> $fileNameTMP 2>&1";
system($cmd);
# print "$cmd\n";

$viewDir = "$productDir/view";
$cmd = "mkdir $viewDir >> $fileNameTMP 2>&1";
system($cmd);
# print "$cmd\n";

$tgzDir = "$productDir/tgz";
$cmd = "mkdir $tgzDir >> $fileNameTMP 2>&1";
system($cmd);
# print "$cmd\n";

#change suffix (arc -> arc.asc, grid -> asc)
foreach $key1 (qw(min max mean idw den)) {
    $key = $key1."arc";
    $key2 = $key1."grid";
    if($$key == 1 || $$key2 == 1)
    {
        $cmd = "mv -f $binDir/$fileName.$key1.arc $binDir/$fileName.$key1.arc.asc >> $fileNameTMP 2>&1";
        system ($cmd);
         # print "$cmd\n";
    }
    $key = $key1."grid";
    if($$key == 1)
    {
        $cmd = "mv -f $binDir/$fileName.$key1.grid $binDir/$fileName.$key1.asc >> $fileNameTMP 2>&1";
        system ($cmd);
         # print "$cmd\n";
    }
}

# copy products to viewDir
foreach $key (qw(min max mean idw den)) {
    $key1 = $key."arc";
    $key2 = $key."grid";

    if($$key1 == 1 || $$key2 == 1)
    {
	$cmd = "cp -f $binDir/$fileName.$key.arc.asc $viewDir/ >> $fileNameTMP 2>&1";
system ($cmd);
 # print "$cmd\n";
	print "http://$host/downloads/kepler/$fileName/view/$fileName.$key.arc.asc\n";
    }
}

#copy products to tgzDir
$tarexists=0;
$tgzfile = "$tgzdir/$fileName.tar.gz";
if (-s $tgzfile) {
    $tarexists = 1;
    $cmd = "gunzip $tgzfile";
    #print "$cmd\n";
    system($cmd);
}

foreach $key1 (qw(min max mean idw den)) {
    $key = $key1."arc";
    if($$key == 1)
    {
        if($tarexists) {
            $cmd = "cd $binDir/; tar -uvf $fileName.tar $fileName.$key1.arc.asc >> $fileNameTMP 2>&1";
	    system($cmd);
	     # print "$cmd\n";
	} else {
	    $cmd = "cd $binDir/; tar -cvf $fileName.tar $fileName.$key1.arc.asc >> $fileNameTMP 2>&1";
	    system($cmd);
	    $tarexists = 1;
	     # print "$cmd\n";
        }
    }
    $key = $key1."grid";
    if($$key == 1)
    {
        if($tarexists) {
    	    $cmd = "cd $binDir/; tar -uvf $fileName.tar $fileName.$key1.asc >> $fileNameTMP 2>&1";
	    system($cmd);
             # print "$cmd\n";
	} else {
	    $cmd = "cd $binDir/; tar -cvf $fileName.tar $fileName.$key1.asc >> $fileNameTMP 2>&1";
	    system($cmd);
	    $tarexists = 1;
	     # print "$cmd\n";
        }
    }
}
$mddir ="/export/downloads/kepler/metadata/";
$mdfile = "metadata$fileName.txt";
$metadata = "$mddir"."$mdfile";

$localmetadata = "$localDataDir"."metadata$fileName.html";
if( -s $localmetadata ) {
  $cmd = "rm -f $localmetadata";
  system($cmd);
}

if (-s $metadata) {
    $cmd = "cd $mddir; tar -uf $binDir/$fileName.tar $mdfile";
    system($cmd);
    $cmd = "rm $metadata";
    system($cmd);
}

$cmd = "cd $binDir/; gzip $fileName.tar >> $fileNameTMP 2>&1";
system($cmd);
# print "$cmd\n";

$cmd = "mv -f $binDir/$fileName.tar.gz $tgzDir/ >> $fileNameTMP 2>&1";
system($cmd);

print "http://$host/downloads/kepler/$fileName/tgz/$fileName.tar.gz\n";

#remove products
if($fileName != ""){
    $cmd = "rm -f $binDir/$fileName.* >> $fileNameTMP 2>&1";
    system($cmd);
#print "$cmd\n";
}
###################################################


###################################################
# Cleaning
#if everything went well
$cmd = "cat $fileNameTMP >> $interpLog";
system($cmd);
#print "$cmd\n";

#$cmd = "cd $directory; rm -f $inputFile.txt $inputFile.txt_ $fileNameTMP";
$cmd = "rm -f $inputFile.txt $inputFile.txt_"; 
system($cmd);
#print "$cmd\n";
###################################################

