#!/usr/bin/perl 

use Pg;
use CGI qw(:all);

$dbmain = 'template1';
$dbname = 'weather';
$dbhost = "15.14.136.78";

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

$conn = Pg::connectdb("dbname=$dbname host=$dbhost  user=$user");
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

#load_location();
load_curobs();

$rain=$rain_tot + $rtot_offset;

print header;
print start_html("Gates Road Weather");
print<<"EoF";
<center><h2>Weather at Mike\'s House</h2></center>
<br>
EoF

printf("<center>");
printf("<b>Observation time:</b> %s<br>\n",$obstime);
printf("<b>Outdoor Temp:</b> %4gF, <b>Indoor Temp:</b> %4gF<br>\n",CtoF($temp_out), CtoF($temp_in));
printf("<b>Outdoor RH:</b> %d%%,  <b>Indoor RH:</b> %d%%<br>\n", $rh_out, $rh_in);
printf("<b>Outdoor dewpt:</b> %4gF, <b>Indoor dewpt:</b> %4gF<br>\n",CtoF($dp_out),CtoF($dp_in));
printf("<b>Wind Chill:</b> %4gF, <b>Barometer:</b> %5g in-Hg<br>\n",CtoF($chill), mbtoin($barometer));
printf("<b>Rain YTD:</b> %4g in, <b>Rain rate:</b> %4g in/hr<br>\n",mmtoin($rain), mmtoin($rain_rate));
printf("<b>Gust:</b> %4g mi/hr, <b>Direction:</b> %d<br>\n",mpstomph($gust_spd), $gust_dir);
printf("<b>Wind Avg</b> %4g mi/hr, <b>Direction:</b> %d<br>\n",mpstomph($wavg_spd), $wavg_dir);
printf("</center><p>\n");

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$year += 1900;
$today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon+1,$mday,$year,0,0,0);

historical($today,"today");

$today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon+1,1,$year,0,0,0);

historical($today,"this month");

print end_html;
exit;

sub historical {
  my($today,$period)=@_;
  
  $result=$conn->exec("SELECT rain_tot FROM wxobs where obstime >= \'$today\';");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $start_rain=getfield(0,'rain_tot');
  }
  $today_rain = $rain - $start_rain;
  
  $result=$conn->exec("SELECT max(rain_rate) FROM wxobs where obstime >= \'$today\';");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $toprate=getfield($tup,'max');
  }
  
  $result=$conn->exec("SELECT obstime FROM wxobs where obstime >= \'$today\' and 
rain_rate = $toprate;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $toptime=getfield($tup,'obstime');
  }
  
  
  printf("<b>Rain <i>$period:</i></b> %4.4g in<br>\n",mmtoin($today_rain));
  printf("<b>Peak rain rate <i>$period:</i></b> %4.4g in/hr at %s<br>\n",mmtoin($toprate),$toptime);
  
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

sub load_location {
  $result=$conn->exec("SELECT * FROM location;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $contact=getfield($tup,'contact_name');
    $phone=getfield($tup,'contact_phone');
    $email=getfield($tup,'contact_email');
    $lat=getfield($tup,'latitude') + 0.0;
    $long=getfield($tup,'longitude') + 0.0;
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
