<?php
session_start();
#
global $doctype, $title, $miner, $port, $readonly, $notify, $rigs;
global $mcast, $mcastexpect, $mcastaddr, $mcastport, $mcastcode;
global $mcastlistport, $mcasttimeout, $mcastretries, $allowgen;
global $rigipsecurity, $rigtotals, $forcerigtotals;
global $socksndtimeoutsec, $sockrcvtimeoutsec;
global $checklastshare, $poolinputs, $hidefields;
global $ignorerefresh, $changerefresh, $autorefresh;
global $allowcustompages, $customsummarypages;
global $miner_font_family, $miner_font_size;
global $bad_font_family, $bad_font_size;
global $colouroverride, $placebuttons, $userlist;
#
$doctype = "<!DOCTYPE html>\n";
#
# See API-README for more details of these variables and how
# to configure miner.php
#
# Web page title
$title = 'Mine';
#
# Set $readonly to true to force miner.php to be readonly
# Set $readonly to false then it will check cgminer 'privileged'
$readonly = false;
#
# Set $userlist to null to allow anyone access or read API-README
$userlist = null;
#
# Set $notify to false to NOT attempt to display the notify command
# Set $notify to true to attempt to display the notify command
$notify = true;
#
# Set $checklastshare to true to do the following checks:
# If a device's last share is 12x expected ago then display as an error
# If a device's last share is 8x expected ago then display as a warning
# If either of the above is true, also display the whole line highlighted
# This assumes shares are 1 difficulty shares
$checklastshare = true;
#
# Set $poolinputs to true to show the input fields for adding a pool
# and changing the pool priorities
# N.B. also if $readonly is true, it will not display the fields
$poolinputs = false;
#
# Set $rigs to an array of your cgminer rigs that are running
#  format: 'IP:Port' or 'Host:Port' or 'Host:Port:Name'
$rigs = array('127.0.0.1:4028');
#
# Set $mcast to true to look for your rigs and ignore $rigs
$mcast = false;
#
# Set $mcastexpect to at least how many rigs you expect it to find
$mcastexpect = 0;
#
# API Multicast address all cgminers are listening on
$mcastaddr = '224.0.0.75';
#
# API Multicast UDP port all cgminers are listening on
$mcastport = 4028;
#
# The code all cgminers expect in the Multicast message sent
$mcastcode = 'FTW';
#
# UDP port cgminers are to reply on (by request)
$mcastlistport = 4027;
#
# Set $mcasttimeout to the number of seconds (floating point)
# to wait for replies to the Multicast message
$mcasttimeout = 1.5;
#
# Set $mcastretries to the number of times to retry the multicast
$mcastretries = 0;
#
# Set $allowgen to true to allow customsummarypages to use 'gen' 
# false means ignore any 'gen' options
$allowgen = false;
#
# Set $rigipsecurity to false to show the IP/Port of the rig
# in the socket error messages and also show the full socket message
$rigipsecurity = true;
#
# Set $rigtotals to true to display totals on the single rig page
# 'false' means no totals (and ignores $forcerigtotals)
# You can force it to always show rig totals when there is only
# one line by setting $forcerigtotals = true;
$rigtotals = true;
$forcerigtotals = false;
#
# These should be OK for most cases
$socksndtimeoutsec = 10;
$sockrcvtimeoutsec = 40;
#
# List of fields NOT to be displayed
# This example would hide the slightly more sensitive pool information
#$hidefields = array('POOL.URL' => 1, 'POOL.User' => 1);
$hidefields = array();
#
# Auto-refresh of the page (in seconds) - integers only
# $ignorerefresh = true/false always ignore refresh parameters
# $changerefresh = true/false show buttons to change the value
# $autorefresh = default value, 0 means dont auto-refresh
$ignorerefresh = false;
$changerefresh = true;
$autorefresh = 0;
#
# Should we allow custom pages?
# (or just completely ignore them and don't display the buttons)
$allowcustompages = true;
#
# OK this is a bit more complex item: Custom Summary Pages
# As mentioned above, see API-README
# see the example below (if there is no matching data, no total will show)
$mobilepage = array(
 'DATE' => null,
 'RIGS' => null,
 'SUMMARY' => array('Elapsed', 'MHS av', 'Found Blocks=Blks', 'Accepted', 'Rejected=Rej', 'Utility'),
 'DEVS+NOTIFY' => array('DEVS.Name=Name', 'DEVS.ID=ID', 'DEVS.Status=Status', 'DEVS.Temperature=Temp',
			'DEVS.MHS av=MHS av', 'DEVS.Accepted=Accept', 'DEVS.Rejected=Rej',
			'DEVS.Utility=Utility', 'NOTIFY.Last Not Well=Not Well'),
 'POOL' => array('POOL', 'Status', 'Accepted', 'Rejected=Rej', 'Last Share Time'));
$mobilesum = array(
 'SUMMARY' => array('MHS av', 'Found Blocks', 'Accepted', 'Rejected', 'Utility'),
 'DEVS+NOTIFY' => array('DEVS.MHS av', 'DEVS.Accepted', 'DEVS.Rejected', 'DEVS.Utility'),
 'POOL' => array('Accepted', 'Rejected'));
#
$statspage = array(
 'DATE' => null,
 'RIGS' => null,
 'SUMMARY' => array('Elapsed', 'MHS av', 'Found Blocks=Blks',
			'Accepted', 'Rejected=Rej', 'Utility',
			'Hardware Errors=HW Errs', 'Network Blocks=Net Blks',
			'Work Utility'),
 'COIN' => array('*'),
 'STATS' => array('*'));
#
$statssum = array(
 'SUMMARY' => array('MHS av', 'Found Blocks', 'Accepted',
			'Rejected', 'Utility', 'Hardware Errors',
			'Work Utility'));
#
$poolspage = array(
 'DATE' => null,
 'RIGS' => null,
 'SUMMARY' => array('Elapsed', 'MHS av', 'Found Blocks=Blks', 'Accepted', 'Rejected=Rej',
			'Utility', 'Hardware Errors=HW Errs', 'Network Blocks=Net Blks',
			'Work Utility'),
 'POOL+STATS' => array('STATS.ID=ID', 'POOL.URL=URL', 'POOL.Difficulty Accepted=Diff Acc',
			'POOL.Difficulty Rejected=Diff Rej',
			'POOL.Has Stratum=Stratum', 'POOL.Stratum Active=StrAct',
			'POOL.Has GBT=GBT', 'STATS.Times Sent=TSent',
			'STATS.Bytes Sent=BSent', 'STATS.Net Bytes Sent=NSent',
			'STATS.Times Recv=TRecv', 'STATS.Bytes Recv=BRecv',
			'STATS.Net Bytes Recv=NRecv', 'GEN.AvShr=AvShr'));
#
$poolssum = array(
 'SUMMARY' => array('MHS av', 'Found Blocks', 'Accepted',
			'Rejected', 'Utility', 'Hardware Errors',
			'Work Utility'),
 'POOL+STATS' => array('POOL.Difficulty Accepted', 'POOL.Difficulty Rejected',
			'STATS.Times Sent', 'STATS.Bytes Sent', 'STATS.Net Bytes Sent',
			'STATS.Times Recv', 'STATS.Bytes Recv', 'STATS.Net Bytes Recv'));
#
$poolsext = array(
 'POOL+STATS' => array(
	'where' => null,
	'group' => array('POOL.URL', 'POOL.Has Stratum', 'POOL.Stratum Active', 'POOL.Has GBT'),
	'calc' => array('POOL.Difficulty Accepted' => 'sum', 'POOL.Difficulty Rejected' => 'sum',
			'STATS.Times Sent' => 'sum', 'STATS.Bytes Sent' => 'sum',
			'STATS.Net Bytes Sent' => 'sum', 'STATS.Times Recv' => 'sum',
			'STATS.Bytes Recv' => 'sum', 'STATS.Net Bytes Recv' => 'sum',
			'POOL.Accepted' => 'sum'),
	'gen' => array('AvShr' => 'round(POOL.Difficulty Accepted/max(POOL.Accepted,1)*100)/100'),
	'having' => array(array('STATS.Bytes Recv', '>', 0)))
);

#
# customsummarypages is an array of these Custom Summary Pages
$customsummarypages = array('Mobile' => array($mobilepage, $mobilesum),
 'Stats' => array($statspage, $statssum),
 'Pools' => array($poolspage, $poolssum, $poolsext));
#
$here = $_SERVER['PHP_SELF'];
#
global $tablebegin, $tableend, $warnfont, $warnoff, $dfmt;
#
$tablebegin = '<tr><td><table border=1 cellpadding=5 cellspacing=0>';
$tableend = '</table></td></tr>';
$warnfont = '<font color=red><b>';
$warnoff = '</b></font>';
$dfmt = 'H:i:s j-M-Y \U\T\CP';
#
$miner_font_family = 'Verdana, Arial, sans-serif, sans';
$miner_font_size = '13pt';
#
$bad_font_family = '"Times New Roman", Times, serif';
$bad_font_size = '18pt';
#
# Edit this or redefine it in myminer.php to change the colour scheme
# See $colourtable below for the list of names
$colouroverride = array();
#
# Where to place the buttons: 'top' 'bot' 'both'
#  anything else means don't show them - case sensitive
$placebuttons = 'top';
#
# This below allows you to put your own settings into a seperate file
# so you don't need to update miner.php with your preferred settings
# every time a new version is released
# Just create the file 'myminer.php' in the same directory as
# 'miner.php' - and put your own settings in there
if (file_exists('myminer.php'))
 include_once('myminer.php');
#
# This is the system default that must always contain all necessary
# colours so it must be a constant
# You can override these values with $colouroverride
# The only one missing is $warnfont
# - which you can override directly anyway
global $colourtable;
$colourtable = array(
	'body bgcolor'		=> '#ecffff',
	'td color'		=> 'blue',
	'td.two color'		=> 'blue',
	'td.two background'	=> '#ecffff',
	'td.h color'		=> 'blue',
	'td.h background'	=> '#c4ffff',
	'td.err color'		=> 'black',
	'td.err background'	=> '#ff3050',
	'td.bad color'		=> 'black',
	'td.bad background'	=> '#ff3050',
	'td.warn color'		=> 'black',
	'td.warn background'	=> '#ffb050',
	'td.sta color'		=> 'green',
	'td.tot color'		=> 'blue',
	'td.tot background'	=> '#fff8f2',
	'td.lst color'		=> 'blue',
	'td.lst background'	=> '#ffffdd',
	'td.hi color'		=> 'blue',
	'td.hi background'	=> '#f6ffff',
	'td.lo color'		=> 'blue',
	'td.lo background'	=> '#deffff'
);
#
# Don't touch these 2
$miner = null;
$port = null;
#
# Ensure it is only ever shown once
global $showndate;
$showndate = false;
#
# For summary page to stop retrying failed rigs
global $rigerror;
$rigerror = array();
#
global $rownum;
$rownum = 0;
#
// Login
global $ses;
$ses = 'rutroh';
#
function getcss($cssname, $dom = false)
{
 global $colourtable, $colouroverride;

 $css = '';
 foreach ($colourtable as $cssdata => $value)
 {
	$cssobj = explode(' ', $cssdata, 2);
	if ($cssobj[0] == $cssname)
	{
		if (isset($colouroverride[$cssdata]))
			$value = $colouroverride[$cssdata];

		if ($dom == true)
			$css .= ' '.$cssobj[1].'='.$value;
		else
			$css .= $cssobj[1].':'.$value.'; ';
	}
 }
 return $css;
}
#
function getdom($domname)
{
 return getcss($domname, true);
}
#
function htmlhead($mcerr, $checkapi, $rig, $pg = null, $noscript = false)
{
 global $doctype, $title, $miner_font_family, $miner_font_size;
 global $bad_font_family, $bad_font_size;
 global $error, $readonly, $poolinputs, $here;
 global $ignorerefresh, $autorefresh;

 $extraparams = '';
 if ($rig != null && $rig != '')
	$extraparams = "&rig=$rig";
 else
	if ($pg != null && $pg != '')
		$extraparams = "&pg=$pg";

 if ($ignorerefresh == true || $autorefresh == 0)
	$refreshmeta = '';
 else
 {
	$url = "$here?ref=$autorefresh$extraparams";
	$refreshmeta = "\n<meta http-equiv='refresh' content='$autorefresh;url=$url'>";
 }

 if ($readonly === false && $checkapi === true)
 {
	$error = null;
	$access = api($rig, 'privileged');
	if ($error != null
	||  !isset($access['STATUS']['STATUS'])
	||  $access['STATUS']['STATUS'] != 'S')
		$readonly = true;
 }
 $miner_font = "font-family:$miner_font_family; font-size:$miner_font_size;";
 $bad_font = "font-family:$bad_font_family; font-size:$bad_font_size;";

 echo "$doctype<html><head>$refreshmeta
<title>$title</title>
<style type='text/css'>
td { $miner_font ".getcss('td')."}
td.two { $miner_font ".getcss('td.two')."}
td.h { $miner_font ".getcss('td.h')."}
td.err { $miner_font ".getcss('td.err')."}
td.bad { $bad_font ".getcss('td.bad')."}
td.warn { $miner_font ".getcss('td.warn')."}
td.sta { $miner_font ".getcss('td.sta')."}
td.tot { $miner_font ".getcss('td.tot')."}
td.lst { $miner_font ".getcss('td.lst')."}
td.hi { $miner_font ".getcss('td.hi')."}
td.lo { $miner_font ".getcss('td.lo')."}
</style>
</head><body".getdom('body').">\n";
if ($noscript === false)
{
echo "<script type='text/javascript'>
function pr(a,m){if(m!=null){if(!confirm(m+'?'))return}window.location='$here?ref=$autorefresh'+a}\n";

if ($ignorerefresh == false)
 echo "function prr(a){if(a){v=document.getElementById('refval').value}else{v=0}window.location='$here?ref='+v+'$extraparams'}\n";

 if ($readonly === false && $checkapi === true)
 {
echo "function prc(a,m){pr('&arg='+a,m)}
function prs(a,r){var c=a.substr(3);var z=c.split('|',2);var m=z[0].substr(0,1).toUpperCase()+z[0].substr(1)+' GPU '+z[1];prc(a+'&rig='+r,m)}
function prs2(a,n,r){var v=document.getElementById('gi'+n).value;var c=a.substr(3);var z=c.split('|',2);var m='Set GPU '+z[1]+' '+z[0].substr(0,1).toUpperCase()+z[0].substr(1)+' to '+v;prc(a+','+v+'&rig='+r,m)}\n";
	if ($poolinputs === true)
		echo "function cbs(s){var t=s.replace(/\\\\/g,'\\\\\\\\'); return t.replace(/,/g, '\\\\,')}\nfunction pla(r){var u=document.getElementById('purl').value;var w=document.getElementById('pwork').value;var p=document.getElementById('ppass').value;pr('&rig='+r+'&arg=addpool|'+cbs(u)+','+cbs(w)+','+cbs(p),'Add Pool '+u)}\nfunction psp(r){var p=document.getElementById('prio').value;pr('&rig='+r+'&arg=poolpriority|'+p,'Set Pool Priorities to '+p)}\n";
 }
echo "</script>\n";
}
?>
<table width=100% height=100% border=0 cellpadding=0 cellspacing=0 summary='Mine'>
<tr><td align=center valign=top>
<table border=0 cellpadding=4 cellspacing=0 summary='Mine'>
<?php
 echo $mcerr;
}
#
function minhead($mcerr = '')
{
 global $readonly;
 $readonly = true;
 htmlhead($mcerr, false, null, null, true);
}
#
global $haderror, $error;
$haderror = false;
$error = null;
#
function mcastrigs()
{
 global $rigs, $mcastexpect, $mcastaddr, $mcastport, $mcastcode;
 global $mcastlistport, $mcasttimeout, $mcastretries, $error;

 $listname = "0.0.0.0";

 $rigs = array();

 $rep_soc = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
 if ($rep_soc === false || $rep_soc == null)
 {
	$msg = "ERR: mcast listen socket create(UDP) failed";
	if ($rigipsecurity === false)
	{
		$error = socket_strerror(socket_last_error());
		$error = "$msg '$error'\n";
	}
	else
		$error = "$msg\n";

	return;
 }

 $res = socket_bind($rep_soc, $listname, $mcastlistport);
 if ($res === false)
 {
	$msg1 = "ERR: mcast listen socket bind(";
	$msg2 = ") failed";
	if ($rigipsecurity === false)
	{
		$error = socket_strerror(socket_last_error());
		$error = "$msg1$listname,$mcastlistport$msg2 '$error'\n";
	}
	else
		$error = "$msg1$msg2\n";

	socket_close($rep_soc);
	return;
 }

 $retries = $mcastretries;
 $doretry = ($retries > 0);
 do
 {
	$mcast_soc = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
	if ($mcast_soc === false || $mcast_soc == null)
	{
		$msg = "ERR: mcast send socket create(UDP) failed";
		if ($rigipsecurity === false)
		{
			$error = socket_strerror(socket_last_error());
			$error = "$msg '$error'\n";
		}
		else
			$error = "$msg\n";

		socket_close($rep_soc);
		return;
	}

	$buf = "cgminer-$mcastcode-$mcastlistport";
	socket_sendto($mcast_soc, $buf, strlen($buf), 0, $mcastaddr, $mcastport);
	socket_close($mcast_soc);

	$stt = microtime(true);
	while (true)
	{
		$got = @socket_recvfrom($rep_soc, $buf, 32, MSG_DONTWAIT, $ip, $p);
		if ($got !== false && $got > 0)
		{
			$ans = explode('-', $buf, 4);
			if (count($ans) >= 3 && $ans[0] == 'cgm' && $ans[1] == 'FTW')
			{
				$rp = intval($ans[2]);

				if (count($ans) > 3)
					$mdes = str_replace("\0", '', $ans[3]);
				else
					$mdes = '';

				if (strlen($mdes) > 0)
					$rig = "$ip:$rp:$mdes";
				else
					$rig = "$ip:$rp";

				if (!in_array($rig, $rigs))
					$rigs[] = $rig;
			}
		}
		if ((microtime(true) - $stt) >= $mcasttimeout)
			break;

		usleep(100000);
	}

	if ($mcastexpect > 0 && count($rigs) >= $mcastexpect)
		$doretry = false;

 } while ($doretry && --$retries > 0);

 socket_close($rep_soc);
}
#
function getrigs()
{
 global $rigs;

 mcastrigs();

 sort($rigs);
}
#
function getsock($rig, $addr, $port)
{
 global $rigipsecurity;
 global $haderror, $error, $socksndtimeoutsec, $sockrcvtimeoutsec;

 $error = null;
 $socket = null;
 $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
 if ($socket === false || $socket === null)
 {
	$haderror = true;
	if ($rigipsecurity === false)
	{
		$error = socket_strerror(socket_last_error());
		$msg = "socket create(TCP) failed";
		$error = "ERR: $msg '$error'\n";
	}
	else
		$error = "ERR: socket create(TCP) failed\n";

	return null;
 }

 // Ignore if this fails since the socket connect may work anyway
 //  and nothing is gained by aborting if the option cannot be set
 //  since we don't know in advance if it can connect
 socket_set_option($socket, SOL_SOCKET, SO_SNDTIMEO, array('sec' => $socksndtimeoutsec, 'usec' => 0));
 socket_set_option($socket, SOL_SOCKET, SO_RCVTIMEO, array('sec' => $sockrcvtimeoutsec, 'usec' => 0));

 $res = socket_connect($socket, $addr, $port);
 if ($res === false)
 {
	$haderror = true;
	if ($rigipsecurity === false)
	{
		$error = socket_strerror(socket_last_error());
		$msg = "socket connect($addr,$port) failed";
		$error = "ERR: $msg '$error'\n";
	}
	else
		$error = "ERR: socket connect($rig) failed\n";

	socket_close($socket);
	return null;
 }
 return $socket;
}
#
function readsockline($socket)
{
 $line = '';
 while (true)
 {
	$byte = socket_read($socket, 1);
	if ($byte === false || $byte === '')
		break;
	if ($byte === "\0")
		break;
	$line .= $byte;
 }
 return $line;
}
#
function api_convert_escape($str)
{
 $res = '';
 $len = strlen($str);
 for ($i = 0; $i < $len; $i++)
 {
	$ch = substr($str, $i, 1);
	if ($ch != '\\' || $i == ($len-1))
		$res .= $ch;
	else
	{
		$i++;
		$ch = substr($str, $i, 1);
		switch ($ch)
		{
		case '|':
			$res .= "\1";
			break;
		case '\\':
			$res .= "\2";
			break;
		case '=':
			$res .= "\3";
			break;
		case ',':
			$res .= "\4";
			break;
		default:
			$res .= $ch;
		}
	}
 }
 return $res;
}
#
function revert($str)
{
 return str_replace(array("\1", "\2", "\3", "\4"), array("|", "\\", "=", ","), $str);
}
#
function api($rig, $cmd)
{
 global $haderror, $error;
 global $miner, $port, $hidefields;

 $socket = getsock($rig, $miner, $port);
 if ($socket != null)
 {
	socket_write($socket, $cmd, strlen($cmd));
	$line = readsockline($socket);
	socket_close($socket);

	if (strlen($line) == 0)
	{
		$haderror = true;
		$error = "WARN: '$cmd' returned nothing\n";
		return $line;
	}

#	print "$cmd returned '$line'\n";

	$line = api_convert_escape($line);

	$data = array();

	$objs = explode('|', $line);
	foreach ($objs as $obj)
	{
		if (strlen($obj) > 0)
		{
			$items = explode(',', $obj);
			$item = $items[0];
			$id = explode('=', $items[0], 2);
			if (count($id) == 1 or !ctype_digit($id[1]))
				$name = $id[0];
			else
				$name = $id[0].$id[1];

			if (strlen($name) == 0)
				$name = 'null';

			$sectionname = preg_replace('/\d/', '', $name);

			if (isset($data[$name]))
			{
				$num = 1;
				while (isset($data[$name.$num]))
					$num++;
				$name .= $num;
			}

			$counter = 0;
			foreach ($items as $item)
			{
				$id = explode('=', $item, 2);

				if (isset($hidefields[$sectionname.'.'.$id[0]]))
					continue;

				if (count($id) == 2)
					$data[$name][$id[0]] = revert($id[1]);
				else
					$data[$name][$counter] = $id[0];

				$counter++;
			}
		}
	}
	return $data;
 }
 return null;
}
#
function getparam($name, $both = false)
{
 $a = null;
 if (isset($_POST[$name]))
	$a = $_POST[$name];

 if (($both ===