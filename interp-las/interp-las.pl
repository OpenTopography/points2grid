#!/usr/bin/perl
use strict;

print "@ARGV\n";

open(METAFILE, ">metadata.xml") || die "can't create metadata.xml";

#generate metadata.xml file when called, based on data passed in parameters
#this subroutine handles both successful and failed runs
sub generateMetadata
{
	my $status = $_[0];
	my $message = $_[1];
	my $rows = $_[2];
	my $cols = $_[3];
	my $cells = $rows * $cols;
	my @files = @_[4..$#_];

	if($status eq "success")
	{
		print METAFILE "<results><status>success</status><type>p2g dem</type><rows>$rows</rows><cols>$cols</cols><cells>$cells</cells>";
		foreach my $s(@files)
		{
			my $ext = $s;
			$ext =~ s/.*\.//;
			if($ext eq "asc" || $ext eq "grid")
			{
				print METAFILE "<$ext>$s</$ext>";
			}elsif($ext eq "gz")
			{
				print METAFILE "<tar><gzip>$s</gzip></tar>";
			}else
			{
				#in unlikely event an unknown extension is found, print as is.
				print METAFILE "<$ext>$s</$ext>";
			} 

		}
		print METAFILE "</results>";
	}else
	{
		#if status = 'failure' or something else
		print METAFILE "<results><status>failure</status><type>error</type><message>$message</message></results>";
		print "$message\n"; #hack
	}

	#we open metadata.xml in beginning so that if it fails, we can exit early.
	#however, once we're done here, we won't be modifying and most likely
	#exiting, so close.
	close(METAFILE);
}

#flatten parameters into a single line for processing with regular expressions
#insert leading and trailing space so that regex can be applied across entire string (beginning, middle, and end)
my $paramLine = " @ARGV ";

#stderr output messages are for debugging and may be commented out
#print STDERR "Original Parameter Line: $paramLine\n";

#input variable will keep track of the name of the input data for later removal
my $input = "";

#opal2 typically places -o following -i while Minh's code because of logic typically puts --input_format after -i.
#for right now, assume that a space followed by a - symbol is the beginning of a new parameter, not http://www.myurl.com/abcdefg -hijklmnop.txt

if($paramLine =~ s/ -i (.*?) -/ -i "$1" -/)
{
	#print STDERR "Modified Parameter Line: $paramLine\n";
	#print STDERR "Input is a file\n";
	#a file DLed by Opal2 has been specified. Wrap filename in double quotes
	$input = $1;
}elsif($paramLine =~ s/ -l (.*?) -/ -i input -/)
{
	#print STDERR "Modified Parameter Line: $paramLine\n";
	#print STDERR "Input is a URL\n";
	#a url has been specified. Wrap url in quotes, download with an external wget, and save to file as 'input'
	$input = "input";
	
	#stdout and stderr from curl saved in @status, while return code is stored in $code
	print "Downloading input from URL '$1'...\n";
	my @status = `wget --no-check-certificate "$1" -O input 2>&1`;
	my $code = $?;
	#print STDERR "wget Code: $code\nwget Status:\n@status\nEnd of Status\n";

	if($code != 0)
	{
		#wget failed: can't find wget | url not valid | etc.
		#print STDERR "wget failed: $code\n";
		generateMetadata("failure", "Error: Could not download data from '$1'\n");
		exit 1;
	}
}else
{
	#print STDERR "Error: Parameter Line could not modified\n";
	generateMetadata("failure", "Error: Parameter Line could not modified");
	exit 1;
}

#remove any leading or trailing whitespaces
$paramLine =~ s/^\s+//;
$paramLine =~ s/\s+$//;

print "Parameters: $paramLine\n";

#set dependencies for running interp, then run interp, passing all parameters in $paramLine
print "Generating DEMs from data...\n";
#/usr/local/apps/gcc-3.4.5/lib/:/home/wsuser/SoftwareCUR/"
my @status = `LD_LIBRARY_PATH="/usr/local/lib";export LD_LIBRARY_PATH;/home/wsuser/Software/interp-las/interp $paramLine`;
#my @status = `export LD_LIBRARY_PATH="/home/wsuser/Software/laszip-2.1.0/lib:/home/wsuser/Software/gdal-1.8.1/lib:/home/wsuser/Software/libgeotiff-1.3.0/lib:/home/wsuser/Software/libLAS1.7.0/lib:/home/wsuser/Software/curl-7.26.0/lib";/home/wsuser/Software/interp-las/interp $paramLine`;
#get status code from interp process
my $code = $?;

print "@status\n";

#process stdout: scrape it for number of rows and columns, and use them to calculate number of cells.
my $interp_output = "@status";
$interp_output =~ s/ GRID_SIZE_X (.*?)\n/ROWS FOUND/;
my $rows = $1;
$interp_output =~ s/ GRID_SIZE_Y (.*?)\n/COLS FOUND/;
my $cols = $1;

#print STDERR "interp Code: $code\ninterp Status:\n@status\nEnd of Status\n";

#remove input, as it can be large and is no longer needed
my @status = `rm -f $input`;
#print STDERR "original input removed\n";

#use code instead of $?, since in all cases input should be deleted, but executing rm will overwrite interp's return code
if($code != 0)
{
	#interp failed
	#print STDERR "interp failed: $code\n";
	generateMetadata("failure", "Error: interp failed ($code)");
	exit 1;
}

print "Creating archives 'dems.tar'...\n";
#print STDERR "creating archive 'dems.tar'...\n";

#create a tarball named 'dems.tar' and compress it
`tar -cvf dems.tar *.grid *.asc 2>/dev/null`;
`/usr/bin/gzip -2 dems.tar`;

if(!opendir(RESULTSDIR, "."))
{
	generateMetadata("failure", "Error: cannot read results directory");
	exit 1;
}

print "Generating metadata...\n";

#generate metadata.xml of successful run
my @params; # = ("success", "", $rows, $cols);

while(my $file = readdir(RESULTSDIR))
{
	if($file ne "stderr.txt" && $file ne "stdout.txt" && $file ne "." && $file ne ".." && $file ne "metadata.xml")
	{
		push(@params, $file);
	}
}

generateMetadata("success", "", $rows, $cols, @params);

print "Process completed\n";
#print STDERR "process completed\n";
