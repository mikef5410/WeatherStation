#!/usr/bin/perl 

# CGI-bin to query the wxd weather database for pertinent stats
# Mike Ferrara  28 Mar 1998
#
# $Id: split-query.cgi 1170 2009-06-23 23:32:10Z mikef $

use Pg;
use CGI qw(:all);
use CGI::Carp qw(fatalsToBrowser);

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$year += 1900;
$mon++;

$q=new CGI;

$dbmain = 'template1';
$dbname = 'weather';
$dbhost = "";

$iam="split-query.cgi";

$post=1 if (request_method eq "POST");

$get=1 if (! $post);

#$get=1 if (request_method eq "GET");

@rose = ('N','NNE','NE','ENE',
	 'E','ESE','SE','SSE',
	 'S','SSW','SW','WSW',
	 'W','WNW','NW','NNW','N');

%fields = ('Nothing' => 'Nothing', 'obstime' => "Observation Time", 'temp_in' => 'Indoor temperature',
	   'temp_out' => "Outdoor temperature", 'rh_in' => 'Indoor relative humidity',
	   'rh_out' => "Outdoor relative humidity", 'dp_in' => 'Indoor dewpoint',
	   'dp_out' => "Outdoor dewpoint", 'rain_tot' => "Rainfall", 'rain_rate' => "Rain rate",
           'gust_spd' => "Wind gust", 'gust_dir' => "Wind gust direction", 
	   'wavg_spd' => "Wind average", 'wavg_dir' => "Wind average direction",
	   'barometer' => "Barometer" );

%wxobs = ('obstime', \$obstime,  'temp_in', \$temp_in, 
	  'temp_out', \$temp_out,  'rh_in',  \$rh_in,
	  'rh_out',  \$rh_out,    'dp_in', \$dp_in,
	  'dp_out', \$dp_out,      'chill', \$chill,
	  'rain_tot', \$rain_tot, 'rain_rate', \$rain_rate,
	  'gust_spd', \$gust_spd, 'gust_dir', \$gust_dir,
	  'wavg_spd', \$wavg_spd, 'wavg_dir', \$wavg_dir,
	  'barometer', \$barometer);
%curobs = %wxobs;
$curobs{'rtot_offset'}=\$rtot_offset;
$curobs{'selector'}=\$selector;

#$user="mikef";
$user="wwwrun";

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

$result=$conn->exec("set datestyle=postgres;");
$resuly=$conn->exec("SET SQL_Inheritance TO OFF;");

#$result=$conn->exec("set enable_seqscan=off;");

print header;

print start_html(-title=>"Gates Road Weather",-style=>{'src'=>'xmystyle.css'},
      -head=>meta({-name=>"viewport",-content=>"width=device-width","-user-scalable"=>"yes"}));


print<<"EoF";
<center><h2>Weather at Mike\'s House</h2></center>
<br>
EoF

if ($get) {
  do_current();
  #put buttons to get weather stats for other time periods
  print center(h3("Custom Query:"));
  print "Extract weather data for period: <BR>";
  print startform('POST',"interval-query.cgi");
  print "<TABLE><TR>";
  print "<TD>From:</TD>";
  print "<TD>",popup_menu(-name=>'start_month',-labels=>\%mon,-values=>[1,2,3,4,5,6,7,8,9,10,11,12],-default=>$mon);
  print textfield(-name=>'start_day',-default=>$mday,-size=>2,-maxlength=>2);
  print ",";
  print textfield(-name=>'start_year',-default=>$year,-size=>4,-maxlength=>4);
  print " at ";
  print textfield(-name=>'start_time',-default=>'0:00',-size=>5,-maxlength=>5),"</TD></TR>";
  print "<TR><TD>To:</TD>";
  print "<TD>",popup_menu(-name=>'end_month',-labels=>\%mon,-values=>[1,2,3,4,5,6,7,8,9,10,11,12],-default=>$mon);
  print textfield(-name=>'end_day',-default=>$mday,-size=>2,-maxlength=>2);
  print ",";
  print textfield(-name=>'end_year',-default=>$year,-size=>4,-maxlength=>4);
  print " at ";
  print textfield(-name=>'end_time',-default=>'23:59',-size=>5,-maxlength=>5),"</TD></TR></TABLE>";
  print "<BR>";
  print "Data to extract: ";
  print popup_menu(-name=>'data1',-labels=>\%fields,-values=>[sort(keys(%fields))],-default=>'Nothing');
  print popup_menu(-name=>'data2',-labels=>\%fields,-values=>[sort(keys(%fields))],-default=>'Nothing');
  print "<BR>";
  print radio_group(-name=>'outputstyle',-values=>['Tabular','Graph'],-default=>'graph',-linebreak=>'true');
  print "<BR>";
  print submit(-name => 'Go');
  print endform;

  print("<HR width=55% size=4>");
  print "<center>";
  print startform('POST', $iam);
  print submit(-name => 'Stats for this month');
  print endform;
  print "</center>";
}


if ($post) {
  #This month
  $today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon,1,$year,0,0,0);
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
  load_location();
  load_curobs();
  
  $rain=$rain_tot + $rtot_offset;
  if ($mon > 5) {
    $season="06/01/$year 00:00";
  } else {
    $yr=$year-1;
    $season="06/01/$yr 00:00";
  }
  $ytd="01/01/$year 00:00";
  $rainstd=interval_rain($season,pg_now());
  $rainytd=interval_rain($ytd,pg_now());
  
  printf("<center><table border=2><caption align=top><B>Current Weather</B></caption>");
  printf("<tr><td colspan=2><b>Observation time</b></td><td colspan=2>%s<br></td></tr>\n",$obstime);
  printf("<tr><td><b>Outdoor Temp </b></td><td> %.1f<i>&#176F</i></td><td><b>Indoor Temp </b></td><td> %.1f<i>&#176F</i></td></tr>\n",CtoF($temp_out), CtoF($temp_in));
  printf("<tr><td><b>Outdoor RH   </b></td><td> %d%%     </td><td><b>Indoor RH   </b></td><td> %d%%     </td></tr>\n", $rh_out, $rh_in);
  printf("<tr><td><b>Outdoor dewpt</b></td><td> %.1f<i>&#176F</i></td><td><b>Indoor dewpt</b></td><td> %.1f<i>&#176F</i></td></tr>\n",CtoF($dp_out),CtoF($dp_in));
  printf("<tr><td><b>Wind Chill   </b></td><td> %.1f<i>&#176F</i></td><td><b>Barometer   </b></td><td> %.2f <i>in-Hg</i></td></tr>\n",CtoF($chill), mbtoin($barometer));
  printf("<tr><td><b>Rain YTD     </b></td><td> %.2f <i>in</i>   </td><td><b>Rain rate   </b></td><td> %.2f <i>in/hr</i></td></tr>\n",mmtoin($rainytd), mmtoin($rain_rate));
  printf("<tr><td><b>Rain STD     </b></td><td> %.2f <i>in</i>   </td><td></td><td></td></tr>\n",mmtoin($rainstd));
  printf("<tr><td><b>Gust         </b></td><td> %.1f <i>mi/hr</i></td><td><b>Direction   </b></td><td> %s       </td></tr>\n",mpstomph($gust_spd), degtorose($gust_dir));
  printf("<tr><td><b>Wind Avg     </b></td><td> %.1f <i>mi/hr</i></td><td><b>Direction   </b></td><td> %s       </td></tr>\n",mpstomph($wavg_spd), degtorose($wavg_dir));
  printf("</table></center><p>\n");
  
  $today=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon,$mday,$year,0,0,0);
  
  print("<HR width=55% size=4>");

  temp_extreme($today,"today");
  gust($today,"today");
  rain_stats($today,"today");

  printf("<p><a href=\"http://forecast.weather.gov/MapClick.php?lat=%g&lon=%g&site=mtr&smap=1&marine=0&unit=0&lg=en\"><b>Get the current forecast for this location</b></a></p>\n",$lat,-1.0 * $long);

}

sub rain_stats {
  my($today,$period)=@_;

  $result=$conn->exec("SELECT rain_tot, rtot_offset FROM current_obs;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $rain=getfield(0,'rain_tot');
    $offset=getfield(0,'rtot_offset');
  }
  
  my $now=pg_now();
  $q="SELECT rain_tot FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\');";
  $result=$conn->exec($q);
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $start_rain=getfield(0,'rain_tot');
  }
  $today_rain = ($rain + $offset) - $start_rain;
  
  $q="SELECT obstime,rain_rate FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\') AND
       rain_rate = (SELECT max(rain_rate) FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\'));";
  $result=$conn->exec($q);
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

  my $now=pg_now();
  $q="SELECT obstime,temp_out FROM wxobs WHERE 
          (obstime >= \'$today\' AND obstime <=\'$now\') AND temp_out = (SELECT
          max(temp_out) FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\' ));";
  $result=$conn->exec($q);
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $toptemp=getfield($tup,'temp_out');
    $toptime=getfield($tup,'obstime');
  }

  $q="SELECT obstime,temp_out FROM wxobs WHERE 
          (obstime >= \'$today\' AND obstime <= \'$now\') AND temp_out = (SELECT
          min(temp_out) FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\') );";
  $result=$conn->exec($q);
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

  my $now=pg_now();
  $q="SELECT obstime,gust_spd,gust_dir FROM wxobs WHERE 
          (obstime >= \'$today\' AND obstime <= \'$now\') AND gust_spd = (SELECT
          max(gust_spd) FROM wxobs where (obstime >= \'$today\' AND obstime <= \'$now\') );";
  $result=$conn->exec($q);
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $topwind=getfield($tup,'gust_spd');
    $topdir=getfield($tup,'gust_dir');
    $toptime=getfield($tup,'obstime');
  }

  printf("<b>Peak wind gust $period:</b> %.1f<i>mph, %s</i> at %s<br>\n",mpstomph($topwind), degtorose($topdir), $toptime);
}

sub pg_date {
  my($mon,$day,$yr,$hr,$min,$sec)=@_;
  $str=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon,$day,$yr,$hour,$min,$sec);
  return($str);
}

sub parse_date {
  my($timestamp)=@_;
  $timestamp=~m,([0-9]+)/([0-9]+)/([0-9+]),;
  my $mon=$1;
  my $day=$2;
  my $yr=$3;

  my $hr=0;
  my $min=0;
  my $sec=0;
  if ($timestamp=~m,([0-9]+):([0-9]+):([0-9]+),) {
    $hr=$1;
    $min=$2;
    $sec=$3;
  }
  return($mon,$day,$yr,$hr,$min,$sec);
}

sub pg_now {
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
    localtime(time);
  $year += 1900;
  $mon++;
  $str=sprintf("%.2d/%.2d/%.4d %.2d:%.2d:%.2d",$mon,$mday,$year,$hour,$min,$sec);
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
  $result=$conn->exec("SELECT * FROM current_obs limit 1;");
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

#
# Index queries are only selected for small ranges ...
#

sub interval_rain {
  my($start,$end)=@_;

  $result=$conn->exec("SELECT rain_tot, rtot_offset FROM current_obs;");
  if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
    $tup=$result->ntuples-1;
    $rain=getfield(0,'rain_tot');
    $offset=getfield(0,'rtot_offset');
  }
  $result=$conn->exec("BEGIN WORK;");

  $startplus=$start;
  do {
    $q="SELECT CAST(\'$startplus\' AS TIMESTAMP) + CAST(\'5 DAYS\' AS INTERVAL);";
    $result=$conn->exec($q);
    if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
      $tup=$result->ntuples-1;
      $startplus=getfield(0,'?column?');
    }

    $q="SELECT obstime,rain_tot FROM wxobs 
      WHERE (obstime >= \'$start\' AND obstime <= \'$startplus\') order by obstime limit 1 ; ";

    $result=$conn->exec($q);

    $tup=$result->ntuples-1;
  } while ($tup<0);
  $start_rain=getfield(0,'rain_tot');

  $endminus=$end;
  do {
    $q="SELECT CAST(\'$endminus\' AS TIMESTAMP) - CAST(\'20 DAYS\' AS INTERVAL);";
    $result=$conn->exec($q);
    if (cmp_eq(PGRES_TUPLES_OK, $result->resultStatus)) {
      $tup=$result->ntuples-1;
      $endminus=getfield(0,'?column?');
    }

    $q="SELECT obstime,rain_tot FROM wxobs 
                       WHERE (obstime >= \'$endminus\' AND obstime <= \'$end\') 
                       ORDER BY obstime USING > limit 1; ";
    $result=$conn->exec($q);

    $tup=$result->ntuples-1;
  } while($tup<0);
  $stop_rain=getfield(0,'rain_tot');

  $result=$conn->exec("END;");
  $rainfall=($stop_rain - $start_rain);
#  printf("Rainfall from $start to $end: $rainfall\n");
  return($rainfall);
}
