#!/usr/bin/perl

use lib '/home/bbs/bin/';
use Time::Local;
use LocalVars;
use Mail::Sender;
use IO::All;
use strict;

my(%water);
sub main
{
    my($fndes, $userid, $mailto, $outmode, $fnsrc);
    chdir($BBSHOME);
    foreach $fndes ( <$JOBSPOOL/water.des.*> ){
	($userid, $mailto, $outmode, $fnsrc) = parsedes($fndes);
	next if( !$userid || !-e $fnsrc );

	$mailto = "$userid.bbs\@$MYHOSTNAME" if( $mailto eq '.');
	undef %water;
	process($fnsrc, "$TMP/water/", $outmode, $userid);
	output("$userid.bbs\@$MYHOSTNAME", $mailto, $mailto =~ /\.bbs/);
	unlink($fndes, $fnsrc);
    }
}

sub parsedes($)
{
    my $t < io($_[0]);
    my $fnsrc = $_[0];
    $fnsrc =~ s/\.des\./\.src\./;
    return (split("\n", $t), $fnsrc);
}

sub process($$$$)
{
    my($fn, $outdir, $outmode, $me) = @_;
    my($cmode, $who, $time, $say, $orig, %LAST, $len) = ();
    open DIN, "<$fn";
    while( <DIN> ){
	next if( !(($cmode, $who, $time, $say, $orig) = parse($_)) || !$who );

	if( $outmode ){
	    $water{$who} .= $orig;
	} else {
	    next if( $say =~ /<<(上|下)站通知>> -- 我(走|來)囉！/ );
	    if( $time - $LAST{$who} > 1800 ){
		$water{$who} .= (scalar localtime($LAST{$who}))."\n\n"
		    if( $LAST{$who} );
		$water{$who} .= scalar localtime($time) . "\n";
	    }
	    
	    $len = max(length($who), length($me)) + 1;
	    $water{$who} .= sprintf("%-${len}s %s\n",
				    ($cmode ? $who : $me).':' ,
				    $say);
	    $LAST{$who} = $time;
	}
    }
    if( $outmode == 0 ){
	$water{$_} .= scalar localtime($LAST{$_})
	    foreach( keys %LAST );
    }
}

sub parse($)
{
    my($str) = @_;
    my($cmode, $who, $year, $month, $day, $hour, $min, $sec, $say);
    $cmode = ($str =~ /^To/) ? 0 : 1;
    ($who, $say, $month, $day, $year, $hour, $min, $sec) =
	$cmode ?
	$str =~ m|★(.+?)\[37;45m\s*(.*).*?\[(\w+)/(\w+)/(\w+) (\w+):(\w+):(\w+)\]| :
	$str =~ m|^To (.+?):\s*(.*)\[(\d+)/(\d+)/(\d+) (\d+):(\d+):(\d+)\]|;
    return ( !(1 <= $month && $month <= 12 &&
	       1 <= $day   && $day   <= 31 &&
	       0 <= $hour  && $hour  <= 23 &&
	       0 <= $min   && $min   <= 59 &&
	       0 <= $sec   && $sec   <= 59 &&
	       1970 <= $year && $year <= 2038) ?
	     () :
	     ($cmode, $who,
	      timelocal($sec, $min, $hour, $day, $month - 1, $year),
	      $say, $_[0]) );
}

sub output
{
    my($from, $tomail, $bbsmail) = @_;
    my $ms = new Mail::Sender{smtp    => $SMTPSERVER,
			      from    => $from,
			      charset => 'big5'};

    foreach( keys %water ){
	$ms->MailMsg({to      => $tomail,
		      subject => "和 $_ 的水球記錄",
		      msg     => $water{$_}});
    }
}

sub max
{
    return $_[0] > $_[1] ? $_[0] : $_[1];
}

main();
1;
