#!/usr/bin/perl

use POSIX;

$wxterm="/dev/ttyS0";
$sleeper=15;

while(1) {
    $time=`date`;
    print("\n$time");


open(WXTERM,"<$wxterm");
$wxfd=fileno(WXTERM);

$wxtermios=POSIX::Termios->new;
$wxtermios->getattr($wxfd);
$wxtermios->setispeed(&POSIX::B9600);
$wxtermios->setattr($wxfd, &POSIX::TCSANOW);

$cflag=$wxtermios->getcflag;
$cflag=($cflag & ~&POSIX::PARENB) | &POSIX::CLOCAL | &POSIX::CREAD | &POSIX::CS8;
$wxtermios->setcflag( $cflag );
$wxtermios->setattr($wxfd, &POSIX::TCSANOW);

$wxtermios->setcc( &POSIX::VMIN, 1);
$wxtermios->setcc( &POSIX::VTIME,0);
$wxtermios->setattr($wxfd, &POSIX::TCSANOW);

$iflag=$wxtermios->getiflag;
$iflag = $iflag & ~&POSIX::IXOFF & ~&POSIX::INPCK & ~&POSIX::ISTRIP | &POSIX::IGNBRK;
$wxtermios->setiflag( $iflag );
$wxtermios->setattr($wxfd, &POSIX::TCSANOW);

$lflag=$wxtermios->getlflag;
$lflag=$lflag & ~&POSIX::ICANON;
$wxtermios->setlflag( $lflag );

$wxtermios->setattr($wxfd, &POSIX::TCSANOW);

#system "stty cread ignbrk -inpck -istrip cs8 -parenb -icanon min 1 time 0 9600  <$wxterm >$wxterm 2>&1";

select STDOUT;
$|=1;



$count=0;
$lcount=0;

while (1) {
    $num=sysread(WXTERM,$in,1);
    next if ($num=0);
    $val=unpack("C",$in);
#    $val=ord($in);
    printf("%0.2x ", $val);
    $count++;
    if ($count==16) {
	print "\n";
	$count=0;
	$lcount++;
    }
    last if ($lcount == 25);
}
    close(WXTERM);
    sleep($sleeper);
}

