#!/usr/bin/perl 

# CGI-bin to query the wxd weather database for interval stats
# Mike Ferrara  28 Mar 1998
#
# $Id$


use Pg;
use CGI qw(:all);

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$mon++;
$year += 1900;
$tmpdir="/home/httpd/html/wxd/tmp";
$dbmain = 'template1';
$dbname = 'weather';
$dbhost = "";
#$dbhost = "mrf-gw";

$q=new CGI;

$iam=$q->self_url;

$post=0; $get=0;
$post=1 if ($q->request_method eq "POST");
$get=1 if (! $post);
init_tables();

if (!$post) {
  #put buttons to get weather stats for other time periods

  print $q->header();   #-expires=>'now');
  print $q->start_html("Gates Road Weather");
  print $q->center(h2("Weather Data Extract")),$q->br();
  print $q->center(h2("Custom Query:"));
  print $q->startform;
  print "<TABLE><TR>";
  print "<TD>From:</TD>";
  print "<TD>",$q->popup_menu(-name=>'start_month',-labels=>\%mon,-values=>[1,2,3,4,5,6,7,8,9,10,11,12],-default=>$mon);
  print $q->textfield(-name=>'start_day',-default=>$mday,-size=>2,-maxlength=>2);
  print ",";
  print $q->textfield(-name=>'start_year',-default=>$year,-size=>4,-maxlength=>4);
  print " at ";
  print $q->textfield(-name=>'start_time',-default=>'0:00',-size=>5,-maxlength=>5),"</TD></TR>";
  print "<TR><TD>To:</TD>";
  print "<TD>",$q->popup_menu(-name=>'end_month',-labels=>\%mon,-values=>[1,2,3,4,5,6,7,8,9,10,11,12],-default=>$mon);
  print $q->textfield(-name=>'end_day',-default=>$mday,-size=>2,-maxlength=>2);
  print ",";
  print $q->textfield(-name=>'end_year',-default=>$year,-size=>4,-maxlength=>4);
  print " at ";
  print $q->textfield(-name=>'end_time',-default=>'23:59',-size=>5,-maxlength=>5),"</TD></TR></TABLE>";
  print "<BR>";
  print "Data to extract: ";
  print $q->popup_menu(-name=>'data1',-labels=>\%fields,-values=>[sort(keys(%fields))],-default=>'Nothing');
  print $q->popup_menu(-name=>'data2',-labels=>\%fields,-values=>[sort(keys(%fields))],-default=>'Nothing');
  print "<BR>";
  print $q->radio_group(-name=>'outputstyle',-values=>['Tabular','Graph'],-default=>'graph',-linebreak=>'true');
  print "<BR>";
  print $q->submit(-name => 'Go');
  print $q->endform;
  
}

#
# Here's where we handle a POST
#

if ($post) {
  print $q->header();
  db_connect();

  $result=$conn->exec("set datestyle=postgres;");
  $resuly=$conn->exec("SET SQL_Inheritance TO OFF;");

  $q->import_names('Q'); #Import all posted data into the 'Q' namespace

  @querylist=("obstime");
  if ($Q::data1 ne "Nothing") {
    push(@querylist,$Q::data1);
  }
  if ($Q::data2 ne "Nothing") {
    push(@querylist,$Q::data2);
  }

  $queries=join(',',@querylist);

  $start=sprintf("%.2d/%.2d/%.4d %s",$Q::start_month,$Q::start_day,$Q::start_year,$Q::start_time);
  $end=sprintf("%.2d/%.2d/%.4d %s",$Q::end_month,$Q::end_day,$Q::end_year,$Q::end_time);

  $y1_label=$fields{$querylist[1]};
  $y1_unit=$units{$querylist[1]};
  $y1_unit="($y1_unit)" if ($y1_unit ne "");

  $y2_label=$fields{$querylist[2]};
  $y2_unit=$units{$querylist[2]};
  $y2_unit="($y2_unit)" if ($y2_unit ne "");

  if ($#querylist > 1) {
    $graph_title="$y1_label and $y2_label from $start to $end";
    $ndata=2;
  } else {
    $graph_title="$y1_label from $start to $end";
    $ndata=1;
  }
  print $q->start_html(-title=>$graph_title, -bgcolor=>"#F0F0F0");
  interval_data($start,$end,$queries);

  if ($Q::outputstyle eq 'Tabular') {
    for ($j=0; $j<$#timevec; $j++) {
      print("$timevec[$j]  $data1vec[$j]  $data2vec[$j]<br>\n");
    }
  } else {
    $tmpdata="/$tmpdir/wxdata$$";
    open(TDAT,">$tmpdata");
    for ($j=0; $j<$#timevec; $j++) {
      print(TDAT "$timevec[$j]  $data1vec[$j]  $data2vec[$j]\n");
    }    
    close(TDAT);
    $giffile=plot_data($tmpdata,$ndata);
    $giffile=~s,/home/httpd/html/wxd/,,;
    print(center("<img src=\"$giffile\">"));
  }
}

print $q->end_html;
exit;


sub pg_now {
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
    localtime(time);
  $year += 1900;
  $str=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon+1,$mday,$year,$hour,$min,$sec);
  return($str);
}

sub CtoF {
  my($c)=@_;
  return(9*$c/5 + 32.0);
}

sub mmtoin {
  my($mm)=@_;
  return($mm/25.4);
}

sub mbtoin {
  my($mb)=@_;
  return($mb*0.0295299875);
}

sub mpstomph {
  my($mps)=@_;
  return($mps*2.23693629);
}

sub degtorose {
  my($deg)=@_;
  my($snap);
  #To snap to 22.5degree bounds: return(22.5*int((deg/22.5)+0.5))
  $snap=int(($deg/22.5)+0.5);
  return($rose[$snap]);
}

sub noconvert {
  my($arg)=@_;
  return($arg);
}

sub load_location {
  $result=$conn->exec("SELECT * FROM location;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $contact=getfield($tup,'contact_name');
    $phone=getfield($tup,'contact_phone');
    $email=getfield($tup,'contact_email');
    $lat=getfield($tup,'latitude') + 0.0;
    $long=getfield($tup,'longitude') + 0.0;
    $elev=getfield($tup,'elevation') + 0.0;
    $street1=getfield($tup,'street1');
    $street2=getfield($tup,'street2');
    $street3=getfield($tup,'street3');
    $city=getfield($tup,'city');
    $state=getfield($tup,'state');
    $country=getfield($tup,'country');
    $postal=getfield($tup,'postal');
  }
}

sub load_curobs {
  $result=$conn->exec("SELECT * FROM current_obs WHERE selector = 1;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    foreach $fname (keys(%curobs)) {
      rgetfield($tup,$fname,$curobs{$fname});
    }
  }
}

sub getfield {
  my($tup,$fname)=@_;
  my $field = $result->fnumber($fname);
  return ($result->getvalue($tup,$field));
}

sub rgetfield {
  my($tup,$fname,$ref) = @_;
  my $field = $result->fnumber($fname);
  $$ref=($result->getvalue($tup,$field));
}

sub get_resp {
  my ($prompt, $default)=@_;
  
  select STDOUT; $|=1;
  print("$prompt [$default] ");
  $in=<>; chop;
  $in=~s/^[\s\t]+//g;
  $in=~s/[\s\t\n]+$//g;
  if ($in=~/^[\s\t]*$/ ) { return($default); }
  return($in);
}

sub get_yn {
  my ($prompt, $default)=@_;
  
  select STDOUT; $|=1;
  print("$prompt [$default] ");
  $in=<>; chop;
  $in=~s/^[\s\t]+//g;
  $in=~s/[\s\t\n]+$//g;
  if ($in=~/^[\s\t]*$/ ) { $in=$default; }
  if ($in=~/^[\s\t]*[Yy1Tt]/) {
    return(1);
  } else {
    return(0);
  }
}    

sub cmp_eq {
  my($cmp,$ret)=@_;
  my $msg;
  my $rval;
  
  if ($cmp == $ret) {
    $rval=1;
  } else {
    $msg = $conn->errorMessage;
    print "error: $cmp, $ret\n$msg\n";
    $rval=0;
  }
  return($rval);
}

sub cmp_ne {
  my($cmp,$ret)=@_;
  my $msg;
  my $rval;
  
  if ($cmp != $ret) {
    $rval=1;
  } else {
    $msg = $conn->errorMessage;
    print "error: $cmp, $ret\n$msg\n";
    $rval=0;
  }
  return($rval);
}


sub db_connect {
$user="mikef";

if ($dbhost eq "") {
    $conn = Pg::connectdb("dbname=$dbname user=$user");
} else {
    $conn = Pg::connectdb("dbname=$dbname host=$dbhost user=$user");
}

if (!cmp_eq(PGRES_CONNECTION_OK, $conn->status)) {
  die("Cannot find database weather!");
}

$db = $conn->db;
#cmp_eq($dbname, $db);

$user = $conn->user;
#cmp_ne("", $user);

$host = $conn->host;
#cmp_ne("", $host);

$port = $conn->port;
#cmp_ne("", $port);
}

sub interval_data {
  my($start,$end,$queries)=@_;


  $result=$conn->exec("BEGIN WORK;");

  $result=$conn->exec("SELECT $queries FROM wxobs 
              WHERE (obstime >= \'$start\' AND obstime <= \'$end\') order by obstime ; ");

  @querylist=split(/,/,$queries);

  $cvt1=$convert{$querylist[1]} if defined($querylist[1]);
  $cvt2=$convert{$querylist[2]} if defined($querylist[2]);

#  print("@querylist<br>\n");

  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
  }

  $rel1=0;
  $rel2=0;


  #Rain totals over an interval are reported as relative to the beginning of 
  #the interval.
  if ($querylist[1] eq "rain_tot") {
    $rel1=&$cvt1(getfield(0,$querylist[1]));
  }
  if ($querylist[2] eq "rain_tot") {
    $rel2=&$cvt1(getfield(0,$querylist[2]));
  }

  for ($j=0; $j<=$tup; $j++) {
    $timestr=getfield($j,$querylist[0]);
    @timeobs=split(/ /,$timestr);
    $timevec[$j]=sprintf("%.2d/%.2d/%.4d %s",$monnum{ucfirst($timeobs[1])},
			 $timeobs[2],$timeobs[4],$timeobs[3]);
    $data1vec[$j]=(&$cvt1(getfield($j,$querylist[1])))-$rel1 if defined ($querylist[1]);
    $data2vec[$j]=(&$cvt2(getfield($j,$querylist[2])))-$rel2 if defined ($querylist[2]);
  }

  $result=$conn->exec("END;");
}  

sub plot_data {
    local($dataf,$ndata)=@_;

    use IPC::Open2;
    $gnuplot="/usr/bin/gnuplot";

    $tgif="$tmpdir/wxplt$$".".png";
    $pid=open2('GNUPLOTR','GNUPLOTW',"$gnuplot");
    print(GNUPLOTW "set grid\n");
    print(GNUPLOTW "set nokey\n");
    print(GNUPLOTW "set data style lines\n");
    print(GNUPLOTW "set xdata time\n");
    print(GNUPLOTW "set y2tics\n") if ($ndata>1);
    print(GNUPLOTW "set timefmt \"%m/%d/%Y %H:%M:%S\" \n");
    print(GNUPLOTW "set key below\n");
    print(GNUPLOTW "set title \"$graph_title\"\n");
    print(GNUPLOTW "set ylabel \"$y1_label $y1_unit\"\n");
    print(GNUPLOTW "set y2label \"$y2_label $y2_unit\"\n") if ($ndata>1);
#    print(GNUPLOTW "set term gif transparent medium interlace\n");
    print(GNUPLOTW "set term png transparent small color\n");
    print(GNUPLOTW "set output \"$tgif\"\n");
    if ($ndata>1) {
      print(GNUPLOTW "plot \"$dataf\" using 1:3 axes x1y1 lw 3 title \"$y1_label\", \"\" using 1:4 axes x1y2 lw 3 title \"$y2_label\" \n");
    } else {
      print(GNUPLOTW "plot \"$dataf\" using 1:3 axes x1y1 lw 3 title \"$y1_label\"\n");
    }
    print(GNUPLOTW "quit\n");
    close(GNUPLOTW);
    close(GNUPLOTR);
    waitpid($pid,0); #wait for gnuplot to exit
    # I smell a HACK! 
    system("echo \"rm -f $tgif\" | at now + 2 minutes ");
    system("rm -f $dataf");
    return("$tgif");
}

#
# Table initializations
#
sub init_tables {
@rose = ('N','NNE','NE','ENE',
	 'E','ESE','SE','SSE',
	 'S','SSW','SW','WSW',
	 'W','WNW','NW','NNW','N');

%mon = ( 1=>'Jan', 2=>'Feb', 3=>'Mar', 4=>'Apr', 5=>'May', 6=>'Jun',
	7=>'Jul', 8=>'Aug', 9=>'Sep', 10=>'Oct', 11=>'Nov', 12=>'Dec');

%monnum = ('Jan'=>1, 'Feb'=>2, 'Mar'=>3, 'Apr'=>4, 'May'=>5, 'Jun'=>6,
	   'Jul'=>7, 'Aug'=>8, 'Sep'=>9, 'Oct'=>10, 'Nov'=>11, 'Dec'=>12);

%fields = ('Nothing' => 'Nothing', 'obstime' => "Observation Time", 'temp_in' => 'Indoor temperature',
	   'temp_out' => "Outdoor temperature", 'rh_in' => 'Indoor relative humidity',
	   'rh_out' => "Outdoor relative humidity", 'dp_in' => 'Indoor dewpoint',
	   'dp_out' => "Outdoor dewpoint", 'rain_tot' => "Rainfall", 'rain_rate' => "Rain rate",
           'gust_spd' => "Wind gust", 'gust_dir' => "Wind gust direction", 
	   'wavg_spd' => "Wind average", 'wavg_dir' => "Wind average direction",
	   'barometer' => "Barometer" );

%units = ('Nothing' => '', 'obstime' => "", 'temp_in' => 'F',
	   'temp_out' => "F", 'rh_in' => '%',
	   'rh_out' => "%", 'dp_in' => 'F',
	   'dp_out' => "F", 'rain_tot' => "in", 'rain_rate' => "in/hr",
           'gust_spd' => "mph", 'gust_dir' => "", 
	   'wavg_spd' => "mph", 'wavg_dir' => "",
	   'barometer' => "inHg" );

%convert = ('temp_in' => \&CtoF, 'temp_out' => \&CtoF, 'rh_in' => \&noconvert,
	      'rh_out' => \&noconvert, 'dp_in'=>\&CtoF, 'dp_out'=>\&CtoF,
	      'rain_tot'=>\&mmtoin, 'rain_rate'=>\&mmtoin, 'gust_spd'=>\&mpstomph,
	      'gust_dir'=>\&noconvert, 'wavg_spd'=>\&mpstomph, 'wavg_dir'=>\&noconvert,
	      'barometer'=>\&mbtoin,'obstime'=>\&noconvert,'Nothing'=>\&noconvert);


%wxobs = ('obstime', \$obstime,  'temp_in', \$temp_in, 
	  'temp_out', \$temp_out,  'rh_in',  \$rh_in,
	  'rh_out',  \$rh_out,    'dp_in', \$dp_in,
	  'dp_out', \$dp_in,      'chill', \$chill,
	  'rain_tot', \$rain_tot, 'rain_rate', \$rain_rate,
	  'gust_spd', \$gust_spd, 'gust_dir', \$gust_dir,
	  'wavg_spd', \$wavg_spd, 'wavg_dir', \$wavg_dir,
	  'barometer', \$barometer);
%curobs = %wxobs;
$curobs{'rtot_offset'}=\$rtot_offset;
$curobs{'selector'}=\$selector;
}
