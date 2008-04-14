#!/usr/bin/perl 

# CGI-bin to query the wxd weather database for pertinent stats
# Mike Ferrara  28 Mar 1998


use Pg;
use CGI qw(:all);

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$year += 1900;


$dbmain = 'template1';
$dbname = 'weather';
$dbhost = "";
#$dbhost = "15.14.136.78";

$iam="split-query.cgi";

$post=1 if (request_method eq "POST");
$get=1 if (request_method eq "GET");

@rose = ('N','NNE','NE','ENE',
	 'E','ESE','SE','SSE',
	 'S','SSW','SW','WSW',
	 'W','WNW','NW','NNW','N');

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

#$dbhost = get_resp("Connect to host running Postgres:",$dbhost);

#$conn = Pg::connectdb("dbname=$dbmain host=$dbhost");
#cmp_eq(PGRES_CONNECTION_OK, $conn->status);

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



print header;
print start_html("Gates Road Weather");
print<<"EoF";
<center><h2>Weather at Mike\'s House</h2></center>
<br>
EoF

if ($get) {
  do_current();
  #put buttons to get weather stats for other time periods
  print("<HR width=55% size=4>");
  print "<center>";
  print startform('POST', $iam);
  print submit(-name => 'Stats for this month');
  print endform;
  print "</center>";
}


if ($post) {
  #This month
  $today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon+1,1,$year,0,0,0);
  print("<center><b>Month to date stats:</b></center>\n");
  temp_extreme($today,"this month\n");
  gust($today,"this month\n");
  rain_stats($today,"this month\n");
}
  

#printf("<HR>");
#temp_extreme($today,"this month");
#rain_stats($today,"this month");

print end_html;
exit;

sub do_current {
  #load_location();
  load_curobs();
  
  $rain=$rain_tot + $rtot_offset;
  
  printf("<center><table border=2><caption align=top><B>Current Weather</B></caption>");
  printf("<tr><td colspan=2><b>Observation time</b></td><td colspan=2>%s<br></td></tr>\n",$obstime);
  printf("<tr><td><b>Outdoor Temp </b></td><td> %.1f<i>&#176F</i></td><td><b>Indoor Temp </b></td><td> %.1f<i>&#176F</i></td></tr>\n",CtoF($temp_out), CtoF($temp_in));
  printf("<tr><td><b>Outdoor RH   </b></td><td> %d%%     </td><td><b>Indoor RH   </b></td><td> %d%%     </td></tr>\n", $rh_out, $rh_in);
  printf("<tr><td><b>Outdoor dewpt</b></td><td> %.1f<i>&#176F</i></td><td><b>Indoor dewpt</b></td><td> %.1f<i>&#176F</i></td></tr>\n",CtoF($dp_out),CtoF($dp_in));
  printf("<tr><td><b>Wind Chill   </b></td><td> %.1f<i>&#176F</i></td><td><b>Barometer   </b></td><td> %.2f <i>in-Hg</i></td></tr>\n",CtoF($chill), mbtoin($barometer));
  printf("<tr><td><b>Rain YTD     </b></td><td> %.2f <i>in</i>   </td><td><b>Rain rate   </b></td><td> %.2f <i>in/hr</i></td></tr>\n",mmtoin($rain), mmtoin($rain_rate));
  printf("<tr><td><b>Gust         </b></td><td> %.1f <i>mi/hr</i></td><td><b>Direction   </b></td><td> %s       </td></tr>\n",mpstomph($gust_spd), degtorose($gust_dir));
  printf("<tr><td><b>Wind Avg     </b></td><td> %.1f <i>mi/hr</i></td><td><b>Direction   </b></td><td> %s       </td></tr>\n",mpstomph($wavg_spd), degtorose($wavg_dir));
  printf("</table></center><p>\n");
  
  $today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon+1,$mday,$year,0,0,0);
  
  print("<HR width=55% size=4>");

  temp_extreme($today,"today");
  gust($today,"today");
  rain_stats($today,"today");
  
}

sub rain_stats {
  my($today,$period)=@_;

  $result=$conn->exec("SELECT rain_tot, rtot_offset FROM current_obs;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $rain=getfield(0,'rain_tot');
    $offset=getfield(0,'rtot_offset');
  }
  
  $result=$conn->exec("SELECT rain_tot FROM wxobs where obstime >= \'$today\';");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $start_rain=getfield(0,'rain_tot');
  }
  $today_rain = ($rain + $offset) - $start_rain;
  
  $result=$conn->exec("SELECT obstime,rain_rate FROM wxobs where obstime >= \'$today\' AND
       rain_rate = (SELECT max(rain_rate) FROM wxobs where obstime >= \'$today\');");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $toprate=getfield($tup,'rain_rate');
    $toptime=getfield($tup,'obstime');
  }
  
  printf("<b>Rain $period:</b> %.2f <i>in</i><br>\n",mmtoin($today_rain));
  if ($toprate) {
    printf("<b>Peak rain rate $period:</b> %.2f <i>in/hr</i> at %s<br>\n",mmtoin($toprate),$toptime);
  }
}

sub temp_extreme {
  my($today,$period)=@_;

  $result=$conn->exec("SELECT obstime,temp_out FROM wxobs WHERE 
          obstime >= \'$today\' AND temp_out = (SELECT
          max(temp_out) FROM wxobs where obstime >= \'$today\' );");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $toptemp=getfield($tup,'temp_out');
    $toptime=getfield($tup,'obstime');
  }

  $result=$conn->exec("SELECT obstime,temp_out FROM wxobs WHERE 
          obstime >= \'$today\' AND temp_out = (SELECT
          min(temp_out) FROM wxobs where obstime >= \'$today\' );");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $mintemp=getfield($tup,'temp_out');
    $mintime=getfield($tup,'obstime');
  }

  printf("<b>High temp $period:</b> %.1f<i>&#176F</i> at %s<br>\n",CtoF($toptemp),$toptime);
  printf("<b>Low  temp $period:</b> %.1f<i>&#176F</i> at %s<br>\n",CtoF($mintemp),$mintime);
}

sub gust {
  my($today,$period)=@_;

  $result=$conn->exec("SELECT obstime,gust_spd,gust_dir FROM wxobs WHERE 
          obstime >= \'$today\' AND gust_spd = (SELECT
          max(gust_spd) FROM wxobs where obstime >= \'$today\' );");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $topwind=getfield($tup,'gust_spd');
    $topdir=getfield($tup,'gust_dir');
    $toptime=getfield($tup,'obstime');
  }

  printf("<b>Peak wind gust $period:</b> %.1f<i>mph, %s</i> at %s<br>\n",mpstomph($topwind), degtorose($topdir), $toptime);
}


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
