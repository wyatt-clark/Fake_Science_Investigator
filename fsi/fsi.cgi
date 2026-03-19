#!/usr/bin/perl

use Carp;
use DBI;
use CGI qw(:standard);
use CGI::Session;
use Lingua::Stem;


# creating a CGI.pm object
  my $cgi = new CGI;

#Wyatt2026: Not sure all this was needed, but this website kept track of visitors
# Stuff that DBI needs to connect to your database
  my $DB        = "DBI:mysql:yourdb";      # data source name (database) 
  my $username  = "youruser";                     
  my $password  = "youruser";


# In order to start use any of the DBI functionality, you must create a database
# connection and store that connection in the database handle object ($dbh).
  $dbh = DBI->connect($DB, $username, $password, {PrintError => 0})
      || die "Could not open database, ", $DBI::errstr;

&print_start();
#&print_form();

my $text = $cgi->param('text');
my $submit = $cgi->param('submit');
my $reset = $cgi->param('reset');


if ($reset) {
    CORE::reset();
    print "<br><br><br>";
} elsif ($submit && $text) { 
    &print_results($text);
} else {
    print "<br><br><br>";
} 

print_rest_page();

# Simply close the database connection
  $dbh->disconnect; 

&print_end;

exit;


###################################################################
######################### Subroutines #############################
###################################################################


# will be called if there is a session id in the cookie "fsi_informatics". It will look in the database 
# and display all the data in the session
sub print_results {
    print "<center><div id=\"results\">";

    my $text_orig = shift;

    my $cheater = test_cheater($text_orig);

    my $error = "<font size=+1 color=red>";
    
    my $session = new CGI::Session("driver:MySQL", undef, {Handle=>$dbh});
    
    
    # the id method returns the session id of the current session
    my $sid = $session->id();
    # the expire method will return the session's expiration time
    my $etime = $session->expire();
    # the ctime method will return the session's creatinon time
    my $ctime = $session->ctime();
    # the atime method will return the last time the session has been activated
    	my $atime = $session->atime();
    # the remote method will return the ip address of the machine the session was created on.
    # there is a way to make sure the session id matches the ip address stored in the database.
    # this is for security reasons, where someone can steal the sid and use it on another computer.
    my $remote = $session->remote_addr();
    
    my $text = lc($text_orig);
    #Wyatt2026: and write to a file!
    # parse out results and print to a file
    my $cleaned = clean_data($text);
    my @clean_ct = split(' ', $cleaned);
    my $cleaned_ct = $#clean_ct + 1;
    my $rounded;
    
    #Wyatt2026: looks like some hacks here

    if($#clean_ct < 100){
	print "Your entry is too short.<br><br>";
    } elsif ($cheater) {
	print "Stop trying to cheat.  You cut and pasted a section of text mutiple times.<br>We implemented some quick patches to fix a bug our very insightful friends at Slashdot pointed out.<br>";
    }
    else{	
    
	my $out = "/Your/Path/fsi/" . $sid . ".txt";
	&print_temp($out, $cleaned);   
	my %words; 
	map{$words{$_}++} @clean_ct;
	my @word_keys = keys %words;
	my $unique = $#word_keys + 1;
	my $last_feat = $unique / $cleaned_ct;
	
    #Wyatt2026: hack to get extreme imbalance of word frequencies?
	if($last_feat < .10 || $last_feat > .8){
	    print "This text had been classified as <h1>INAUTHENTIC</h1><br>";
	}
	else{		
        
	    my $class = classify_text($out, $last_feat);
	    my @beta = (1.7778122e+00, 5.8917323e-01);
	    
	    my $beta0 = (-1 * $beta[0]);
	    my $y = ($beta0 * ($class + $beta[1]));
	    
	    my $class = (1 / (1 + exp($y))) * 100;
	    system("rm -f $out");
	    
	    $rounded = sprintf("%.1f", $class);
	    
	    
	    
	    if($rounded > 50){
		print "This text had been classified as <br><font size=+2><i>AUTHENTIC</i></font><br>";	
		print "with a <font size=+1><i>$rounded" . "%</i></font> chance of being an authentic paper<br>";
	    }
	    else{
		print "This text had been classified as <br><font size=+2><i>INAUTHENTIC</i></font><br>";
		print "with a <font size=+1><i>$rounded" . "%</i></font> chance of being authentic text<br>";
	    }
	    
	}
    }
    
    
    # if the data and file parameters have been entered, then store them in the database
    # under the current session      
    
    

    if ($text && $cleaned && $rounded && $#clean_ct > 100) {
	$text =~ s/\W//g;
	$text =~ s/\'//g;
	$text =~ s/\"//g;
	$text =~ s/\;//g;
	$text =~ s/\://g;
	
	$text =~ s/\&//g;
	
	my $sessText = $session->param('original');
	
	if ($sessText ne $text) {
	    $session->param("length", $#clean_ct);
	    $session->param("original", $text);
	    $session->param("cleaned", $cleaned);
	    $session->param("class", "$class");
	}
    } 

    print "</div></center>";
}


# simply prints the  form
sub print_form {
    print     "<p>Paste any text in the textbox.   The chance that your submission is a human-written authentic scientific document will be output.  Text over 50% chance will be classified as authentic.</p>", 
    "<center><form name=\"fsi\" action=\"/cgi-bin/fsi/fsi.cgi\" method=\"POST\">",
    $cgi->br,
    $cgi->textarea(-name=>'text',
		   -default=>'cut and paste text here',
		   -rows=>10,
		   -columns=>50),
    "<br><input type=\"submit\" name=\"submit\" value=\"Submit\"> &nbsp",
    "<input type=\"submit\" name=\"reset\" value=\"Reset\">",
    "</form>",
    "</center><br>";

}



# simply opens a file and returns an array of the data
sub open_file {
    my $file = shift;
    my @data = ();
    
    my $line = "";
    
    while(<$file>) {
        $line = $_;
        chomp($line);
        push(@data, $line);
    }
    
    return @data;
}



sub clean_data {
    my $temp = shift;
    
    # get the stop words file
	open(FILE, "stop_words.txt");
	my @stops = <FILE>;
	close FILE;
    chomp(@stops);
    my %s = map { $_, 1 } @stops;
    $s{"\n"} = 1;
    
    #Wyatt2026: this is a bit hacky, but obviously had some repeated text in the corpora I collected
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
    
    return $text;
}


sub print_start {
    print $cgi->header(-meta=>{'keywords'=>'compression authentic inauthentic paper detection fake',
			       'description'=>'inauthentic paper detection program'}),
    $cgi->start_html(-title=>'FSI Indiana University School of Informatics',
		     -style=>'http://montana.informatics.indiana.edu/fsi/style.css',
		     -head=>Link({-rel=>'shortcut icon',
                                  -href=>'http://montana.informatics.indiana.edu/fsi/mag_glass.gif'}));


    print "<div id=\"mainwrapper\"><div id=\"header\"><img src=\"http://montana.informatics.indiana.edu/fsi/nothing.png\" alt=\"Fake Science Investigator\" /></div>",
    "<div id=\"columnswrapper\"><div id=\"content\"><div id=\"contentinner\">", 
    "<h1>Welcome to the Inauthentic Paper Detector</h1><br>";

    print_form();
}

#Wyatt2026: aw man, this brings back some memories.
sub print_rest_page {
    print "</div></div><div id=\"nav\"><ul>",
    "<li><a href=\"http://montana.informatics.indiana.edu/cgi-bin/fsi/fsi.cgi\">Detect Inauthentic Paper</a></li>",
    "<li><a href=\"http://montana.informatics.indiana.edu/fsi/about.html\">About</a></li>",
    "<li><a href=\"http://montana.informatics.indiana.edu/fsi/siampaper.pdf\">Publication SIAM Conference on Data Mining (2006)</a></li>",
    "<li><a href=\"http://montana.informatics.indiana.edu/fsi/references.html\">References</a></li>",
    "<li><a href=\"http://montana.informatics.indiana.edu/fsi/fake.html\">Can You Spot the Fake?</a></li>",
    "<li><a href=\"http://www.siam.org/meetings/sdm06/index.htm\">2006 SIAM Conference on Data Mining</a></li>",
    "<li><h3>People</h3></li><ul>",
    "<li><img src=\"http://montana.informatics.indiana.edu/fsi/bullet1.png\"> <a href=\"http://spock.informatics.indiana.edu/~wtclark/wyatt.htm\">Wyatt Clark</a></li>",
    "<li><img src=\"http://montana.informatics.indiana.edu/fsi/bullet1.png\"> <a href=\"http://biokdd.informatics.indiana.edu/jccostel/\">Jim Costello</a></li>",
    "<li><img src=\"http://montana.informatics.indiana.edu/fsi/bullet1.png\"> <a href=\"http://www.informatics.indiana.edu/dalkilic\">Mehmet Dalkilic</a></li>",
    "<li><img src=\"http://montana.informatics.indiana.edu/fsi/bullet1.png\"> <a href=\"http://www.informatics.indiana.edu/predrag\">Predrag Radivojac</a></li>",
    "</ul></ul>",
    "</div></div></div>";
}


sub print_end {
    print "<div id=\"footer\">",
    "<h2>Hosted by <a href=\"http://www.iub.edu\">Indiana University</a> <a href=\"http://www.informatics.indiana.edu\">School of Informatics</a></h2>",
    "<h2>&#169; FSI 2005</h2><h2>Detecting Fake Science since 2005</h2><h2>webmaster: jccostel [at] indiana [dot] edu </h2>",
    "</div>",
    $cgi->end_html;
}


#wyatt  here is where you can call the code.  $text is the location of a file that contains the rsed and cleaned text.  You should be able to call that as the input to the svm
sub classify_text {
    my $text = shift;
    my $last_feature = shift;    
    my $out = $text . "comp";

    my @features;
    for (my $j = 2; $j < 2050; $j = $j*2){
    #Wyatt2026: not sure these executables will work widely anymore.
    #I am including the source for each method though.
	system("./BenderWolf $text $j $out");
	open(FILE, "$out");
	my @data = <FILE>;
	close (FILE);
	system("rm -f $out");
	push @features, $data[0];
	
	system("./LempelZiv $text $j $out");
	open(FILE, "$out");
	my @data1 = <FILE>;
	close (FILE);
	system("rm -f $out");
	push @features, $data1[0];
	
    }
    #Wyatt2026: here I hand typed in the normalization values from training data (which unfortunately I don't have anymore)
    push @features, $last_feature;
	 my @std = (6.5898693e-02, 6.2361968e-02, 6.9920757e-02, 6.6519708e-02, 1.6605363e+02, 3.6480542e-01, 3.8757118e+02, 2.6520192e+00, 3.9355802e+02, 4.7016526e+00, 3.8137204e+02, 8.0016944e+00, 3.7055891e+02, 1.3735651e+01, 3.6331834e+02, 2.3564054e+01, 3.5227866e+02, 4.0523710e+01, 3.4484739e+02, 6.6188127e+01, 3.3324484e+02, 1.0344040e+02, 2.0502967e-01);

 	my @mean = (9.2861807e-01, 9.2881097e-01, 9.7734267e-01, 9.6257875e-01, 6.2842523e+00, 1.0353227e+00, 6.9491800e+01, 1.5504236e+00, 7.5685059e+01, 2.1247512e+00, 7.3828281e+01, 2.9616095e+00, 7.1678388e+01, 4.3466275e+00, 7.0187689e+01, 6.6413961e+00, 7.0778247e+01, 1.1788733e+01, 6.9440412e+01, 1.8012141e+01, 6.8974131e+01, 2.7532111e+01, 3.4445770e-01);

    my @normalized = ();
    for(my $i=0; $i<=$#features; $i++) {
	$normalized[$i] = ($features[$i] - $mean[$i])/$std[$i];

    }
    my @outt = ();
    push @outt, "0 ";
    for(my $i=1; $i<=$#normalized+1; $i++) {	
        push @outt, "$i:$normalized[$i-1] ";
    }
    push @outt, " \n";
    
    my $finally = join " ", @outt;
    my $matrix = "$text" . "matrix";
    &print_temp($matrix, $finally);
    my $results = "$text" . "rizzles";
    #Wyatt2026: also, good luck running this executable. back in the day, we didn't have language native learning algorithms!
    system("./svm_classify -v 0 $matrix predictor $results");


#        my @vals = open_file($results);     
    open (FILE, "$results");
    my @vals = <FILE>;
    close FILE;
    system("rm -f $matrix");
    system("rm -f $results");
    my $val = $vals[0];
    
    return $val;
}


sub print_temp {
    my $id = shift;
    my $text = shift;

    open(OUT, ">$id") || die;
    print OUT $text;
    close OUT;
} 




sub test_cheater {
    my $text = shift;


    my @words = split(/\s+/, $text);

    my %hash = ();
    foreach(@words) {
	$hash{$_}++;
    }

    my $ones = 0;
    while(my ($k, $v) = each(%hash)) {
	$ones++ if $v == 1;
    }


    my $total = keys(%hash);
    my $perc = $ones / $total;
    return 0 if $perc > .05;

    return 1;
}
