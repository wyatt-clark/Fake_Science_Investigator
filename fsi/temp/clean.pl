#!/usr/bin/perl -w
use strict;
use Lingua::Stem;
#$dir1 is the directory that is to be rad from
my $dir1 = $ARGV[0];
#$dir2 is the directory that is to be reaad from
my $dir2 = $ARGV[1];
print "$dir1\n";
#this block of code reads the stop words into a file
open (FILE, "stop_words.txt") or die ("couldn't stop words fil open file\n");
my @stops = <FILE>;
close (FILE);
chomp @stops;
my %s = map { $_, 1 } @stops;
$s{"\n"} = 1;


my @files = grep "$_", glob "$dir1*";

#iterate over the files 
foreach my $file(@files){
	#this block of code open a file and reads it into $temp
	open (FILE, "$file") or die ("couldn't open file\n");
	my $temp;
	while(my $line = <FILE>){
		$temp.=lc($line);
	}
	#this line rejoins words that heve been split and hypenated at the end of a line
	$temp =~ s/[-{1}][\s*\n+]{1}//gi;


	#substitute whitespaces greater than two with one white space
	$temp =~ s/\s{2,}/ /g;

	$temp =~ s/Download a Postscript or PDF version of this paper//gi; 
	$temp =~ s/Download all the files for this paper as a gzipped tar archive//gi;

	$temp =~ s/SummaryPlus//gi;
	$temp =~ s/Abstract \+ References//gi;
	$temp =~ s/\$Order Document//gi;
	$temp =~ s/View this table://gi;
	$temp =~ s/Abstract\-EMBASE//gi;
	$temp =~ s/Abstract\-PsycINFO//gi;
	$temp =~ s/Abstract\-MEDLINE//gi;
	$temp =~ s/Abstract\-GEOBASE//gi;
	$temp =~ s/Abstract\-EconLit//gi;
	$temp =~ s/Abstract\-Elsevier BIOBASE//gi;
	$temp =~ s/Full Text via CrossRef//gi;
	$temp =~ s/MathSciNet//gi;
	$temp =~ s/Full Text//gi;
	$temp =~ s/Links \|//gi;
	$temp =~ s/PDF//gi;
	$temp =~ s/Abstract//gi;
	$temp =~ s/Summary//gi;
	$temp =~ s/Back to the SCIgen homepage//gi;
	$temp =~ s/Generate another one//gi;



	
	#replace designated symbols with a space
	$temp =~ s/\W/ /g;

	#split temp into an array on white space
	my @textAr = split( ' ', $temp);

	#remove worde that are less than 3 or greater than 16 in length
	@textAr = grep {length($_) > 2} @textAr;
	@textAr = grep {length($_) < 20} @textAr;

	#remove all words that are only numbers
	@textAr = grep {!/^\d+$/} @textAr;
	

	#remove stop words
	@textAr = grep { not exists $s{$_} } @textAr;

	#this block stems the text
	my $stemmer = Lingua::Stem->new(-locale => 'en');
	my $stemmed = $stemmer->stem(@textAr);
	my @t = @$stemmed;

	#turn the array of words back into a long string
	my $text = join " ", @t;
	
	#remove any whitespace that is longer than one again
	$text =~ s/\s{2,}/ /g;
	my @path = split '/', $file;
	my $fn = $path[$#path];
	my $out = "$dir2"."$fn";
	#write out to a file
        open (FILE, ">$out");
	print FILE ("$text");
	close FILE;

}
