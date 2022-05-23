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

 if (($both === true) and ($a === null))
 {
	if (isset($_GET[$name]))
		$a = $_GET[$name];
 }

 if ($a == '' || $a == null)
	return null;

 // limit to 1K just to be safe
 return substr($a, 0, 1024);
}
#
function newtable()
{
 global $tablebegin, $rownum;
 echo $tablebegin;
 $rownum = 0;
}
#
function newrow()
{
 echo '<tr>';
}
#
function othrow($row)
{
 return "<tr>$row</tr>";
}
#
function otherrow($row)
{
 echo othrow($row);
}
#
function endrow()
{
 global $rownum;
 echo '</tr>';
 $rownum++;
}
#
function endtable()
{
 global $tableend;
 echo $tableend;
}
#
function classlastshare($when, $alldata, $warnclass, $errorclass)
{
 global $checklastshare;

 if ($checklastshare === false)
	return '';

 if ($when == 0)
	return '';

 if (!isset($alldata['MHS av']))
	return '';

 if ($alldata['MHS av'] == 0)
	return '';

 if (!isset($alldata['Last Share Time']))
	return '';

 if (!isset($alldata['Last Share Difficulty']))
	return '';

 $expected = pow(2, 32) / ($alldata['MHS av'] * pow(10, 6));

 // If the share difficulty changes while waiting on a share,
 // this calculation will of course be incorrect
 $expected *= $alldata['Last Share Difficulty'];

 $howlong = $when - $alldata['Last Share Time'];
 if ($howlong < 1)
	$howlong = 1;

 if ($howlong > ($expected * 12))
	return $errorclass;

 if ($howlong > ($expected * 8))
	return $warnclass;

 return '';
}
#
function endzero($num)
{
 $rep = preg_replace('/0*$/', '', $num);
 if ($rep === '')
	$rep = '0';
 return $rep;
}
#
function fmt($section, $name, $value, $when, $alldata)
{
 global $dfmt, $rownum;

 if ($alldata == null)
	$alldata = array();

 $errorclass = ' class=err';
 $warnclass = ' class=warn';
 $lstclass = ' class=lst';
 $hiclass = ' class=hi';
 $loclass = ' class=lo';
 $c2class = ' class=two';
 $totclass = ' class=tot';
 $b = '&nbsp;';

 $ret = $value;
 $class = '';

 $nams = explode('.', $name);
 if (count($nams) > 1)
	$name = $nams[count($nams)-1];

 if ($value === null)
	$ret = $b;
 else
	switch ($section.'.'.$name)
	{
	case 'GPU.Last Share Time':
	case 'PGA.Last Share Time':
	case 'ASC.Last Share Time':
	case 'DEVS.Last Share Time':
		if ($value == 0
		||  (isset($alldata['Last Share Pool']) && $alldata['Last Share Pool'] == -1))
		{
			$ret = 'Never';
			$class = $warnclass;
		}
		else
		{
			$ret = date('H:i:s', $value);
			$class = classlastshare($when, $alldata, $warnclass, $errorclass);
		}
		break;
	case 'GPU.Last Valid Work':
	case 'PGA.Last Valid Work':
	case 'ASC.Last Valid Work':
	case 'DEVS.Last Valid Work':
		if ($value == 0)
			$ret = 'Never';
		else
			$ret = ($value - $when) . 's';
		break;
	case 'POOL.Last Share Time':
		if ($value == 0)
			$ret = 'Never';
		else
			$ret = date('H:i:s d-M', $value);
		break;
	case 'GPU.Last Share Pool':
	case 'PGA.Last Share Pool':
	case 'ASC.Last Share Pool':
	case 'DEVS.Last Share Pool':
		if ($value == -1)
		{
			$ret = 'None';
			$class = $warnclass;
		}
		break;
	case 'SUMMARY.Elapsed':
	case 'STATS.Elapsed':
		$s = $value % 60;
		$value -= $s;
		$value /= 60;
		if ($value == 0)
			$ret = $s.'s';
		else
		{
			$m = $value % 60;
			$value -= $m;
			$value /= 60;
			if ($value == 0)
				$ret = sprintf("%dm$b%02ds", $m, $s);
			else
			{
				$h = $value % 24;
				$value -= $h;
				$value /= 24;
				if ($value == 0)
					$ret = sprintf("%dh$b%02dm$b%02ds", $h, $m, $s);
				else
				{
					if ($value == 1)
						$days = '';
					else
						$days = 's';
	
					$ret = sprintf("%dday$days$b%02dh$b%02dm$b%02ds", $value, $h, $m, $s);
				}
			}
		}
		break;
	case 'NOTIFY.Last Well':
		if ($value == '0')
		{
			$ret = 'Never';
			$class = $warnclass;
		}
		else
			$ret = date('H:i:s', $value);
		break;
	case 'NOTIFY.Last Not Well':
		if ($value == '0')
			$ret = 'Never';
		else
		{
			$ret = date('H:i:s', $value);
			$class = $errorclass;
		}
		break;
	case 'NOTIFY.Reason Not Well':
		if ($value != 'None')
			$class = $errorclass;
		break;
	case 'GPU.Utility':
	case 'PGA.Utility':
	case 'ASC.Utility':
	case 'DEVS.Utility':
	case 'SUMMARY.Utility':
	case 'total.Utility':
		$ret = $value.'/m';
		if ($value == 0)
			$class = $errorclass;
		else
			if (isset($alldata['Difficulty Accepted'])
			&&  isset($alldata['Accepted'])
			&&  isset($alldata['MHS av'])
			&&  ($alldata['Difficulty Accepted'] > 0)
			&&  ($alldata['Accepted'] > 0))
			{
				$expected = 60 * $alldata['MHS av'] * (pow(10, 6) / pow(2, 32));
				if ($expected == 0)
					$expected = 0.000001; // 1 H/s

				$da = $alldata['Difficulty Accepted'];
				$a = $alldata['Accepted'];
				$expected /= ($da / $a);

				$ratio = $value / $expected;
				if ($ratio < 0.9)
					$class = $loclass;
				else
					if ($ratio > 1.1)
						$class = $hiclass;
			}
		break;
	case 'SUMMARY.Work Utility':
	case 'total.Work Utility':
		$ret = $value.'/m';
		break;
	case 'GPU.Temperature':
	case 'PGA.Temperature':
	case 'ASC.Temperature':
	case 'DEVS.Temperature':
		$ret = $value.'&deg;C';
		if (!isset($alldata['GPU']))
		{
			if ($value == 0)
				$ret = '&nbsp;';
			break;
		}
	case 'GPU.GPU Clock':
	case 'DEVS.GPU Clock':
	case 'GPU.Memory Clock':
	case 'DEVS.Memory Clock':
	case 'GPU.GPU Voltage':
	case 'DEVS.GPU Voltage':
	case 'GPU.GPU Activity':
	case 'DEVS.GPU Activity':
		if ($value == 0)
			$class = $warnclass;
		break;
	case 'GPU.Fan Percent':
	case 'DEVS.Fan Percent':
		if ($value == 0)
			$class = $warnclass;
		else
		{
			if ($value == 100)
				$class = $errorclass;
			else
				if ($value > 85)
					$class = $warnclass;
		}
		break;
	case 'GPU.Fan Speed':
	case 'DEVS.Fan Speed':
		if ($value == 0)
			$class = $warnclass;
		else
			if (isset($alldata['Fan Percent']))
			{
				$test = $alldata['Fan Percent'];
				if ($test == 100)
					$class = $errorclass;
				else
					if ($test > 85)
						$class = $warnclass;
			}
		break;
	case 'GPU.MHS av':
	case 'PGA.MHS av':
	case 'ASC.MHS av':
	case 'DEVS.MHS av':
	case 'SUMMARY.MHS av':
	case 'total.MHS av':
		$parts = explode('.', $value, 2);
		if (count($parts) == 1)
			$dec = '';
		else
			$dec = '.'.$parts[1];
		$ret = number_format((float)$parts[0]).$dec;

		if ($value == 0)
			$class = $errorclass;
		else
			if (isset($alldata['Difficulty Accepted'])
			&&  isset($alldata['Accepted'])
			&&  isset($alldata['Utility'])
			&&  ($alldata['Difficulty Accepted'] > 0)
			&&  ($alldata['Accepted'] > 0))
			{
				$expected = 60 * $value * (pow(10, 6) / pow(2, 32));
				if ($expected == 0)
					$expected = 0.000001; // 1 H/s

				$da = $alldata['Difficulty Accepted'];
				$a = $alldata['Accepted'];
				$expected /= ($da / $a);

				$ratio = $alldata['Utility'] / $expected;
				if ($ratio < 0.9)
					$class = $hiclass;
				else
					if ($ratio > 1.1)
						$class = $loclass;
			}
		break;
	case 'GPU.Total MH':
	case 'PGA.Total MH':
	case 'ASC.Total MH':
	case 'DEVS.Total MH':
	case 'SUMMARY.Total MH':
	case 'total.Total MH':
	case 'SUMMARY.Getworks':
	case 'POOL.Getworks':
	case 'total.Getworks':
	case 'GPU.Accepted':
	case 'PGA.Accepted':
	case 'ASC.Accepted':
	case 'DEVS.Accepted':
	case 'SUMMARY.Accepted':
	case 'POOL.Accepted':
	case 'total.Accepted':
	case 'GPU.Rejected':
	case 'PGA.Rejected':
	case 'ASC.Rejected':
	case 'DEVS.Rejected':
	case 'SUMMARY.Rejected':
	case 'POOL.Rejected':
	case 'total.Rejected':
	case 'SUMMARY.Local Work':
	case 'total.Local Work':
	case 'SUMMARY.Discarded':
	case 'POOL.Discarded':
	case 'total.Discarded':
	case 'POOL.Diff1 Shares':
	case 'total.Diff1 Shares':
	case 'GPU.Diff1 Work':
	case 'PGA.Diff1 Work':
	case 'ASC.Diff1 Work':
	case 'total.Diff1 Work':
	case 'STATS.Times Sent':
	case 'STATS.Bytes Sent':
	case 'STATS.Net Bytes Sent':
	case 'STATS.Times Recv':
	case 'STATS.Bytes Recv':
	case 'STATS.Net Bytes Recv':
	case 'total.Times Sent':
	case 'total.Bytes Sent':
	case 'total.Net Bytes Sent':
	case 'total.Times Recv':
	case 'total.Bytes Recv':
	case 'total.Net Bytes Recv':
		$parts = explode('.', $value, 2);
		if (count($parts) == 1)
			$dec = '';
		else
			$dec = '.'.$parts[1];
		$ret = number_format((float)$parts[0]).$dec;
		break;
	case 'STATS.Hs':
	case 'STATS.W':
	case 'STATS.history_time':
	case 'STATS.Pool Wait':
	case 'STATS.Pool Max':
	case 'STATS.Pool Min':
	case 'STATS.Pool Av':
	case 'STATS.Min Diff':
	case 'STATS.Max Diff':
	case 'STATS.Work Diff':
		$parts = explode('.', $value, 2);
		if (count($parts) == 1)
			$dec = '';
		else
			$dec = '.'.endzero($parts[1]);
		$ret = number_format((float)$parts[0]).$dec;
		break;
	case 'GPU.Status':
	case 'PGA.Status':
	case 'ASC.Status':
	case 'DEVS.Status':
	case 'POOL.Status':
		if ($value != 'Alive')
			$class = $errorclass;
		break;
	case 'GPU.Enabled':
	case 'PGA.Enabled':
	case 'ASC.Enabled':
	case 'ASC.Enabled':
	case 'DEVS.Enabled':
		if ($value != 'Y')
			$class = $warnclass;
		break;
	case 'STATUS.When':
	case 'COIN.Current Block Time':
		$ret = date($dfmt, $value);
		break;
	case 'BUTTON.Rig':
	case 'BUTTON.Pool':
	case 'BUTTON.GPU':
		$ret = $value;
		break;
	case 'SUMMARY.Difficulty Accepted':
	case 'GPU.Difficulty Accepted':
	case 'PGA.Difficulty Accepted':
	case 'ASC.Difficulty Accepted':
	case 'DEVS.Difficulty Accepted':
	case 'POOL.Difficulty Accepted':
	case 'total.Difficulty Accepted':
	case 'SUMMARY.Difficulty Rejected':
	case 'GPU.Difficulty Rejected':
	case 'PGA.Difficulty Rejected':
	case 'ASC.Difficulty Rejected':
	case 'DEVS.Difficulty Rejected':
	case 'POOL.Difficulty Rejected':
	case 'total.Difficulty Rejected':
	case 'SUMMARY.Difficulty Stale':
	case 'POOL.Difficulty Stale':
	case 'total.Difficulty Stale':
	case 'GPU.Last Share Difficulty':
	case 'PGA.Last Share Difficulty':
	case 'ASC.Last Share Difficulty':
	case 'DEVS.Last Share Difficulty':
	case 'POOL.Last Share Difficulty':
		if ($value != '')
			$ret = number_format((float)$value, 2);
		break;
	case 'DEVS.Device Hardware%':
	case 'DEVS.Device Rejected%':
	case 'ASC.Device Hardware%':
	case 'ASC.Device Rejected%':
	case 'PGA.Device Hardware%':
	case 'PGA.Device Rejected%':
	case 'GPU.Device Hardware%':
	case 'GPU.Device Rejected%':
	case 'POOL.Pool Rejected%':
	case 'POOL.Pool Stale%':
	case 'SUMMARY.Device Hardware%':
	case 'SUMMARY.Device Rejected%':
	case 'SUMMARY.Pool Rejected%':
	case 'SUMMARY.Pool Stale%':
		if ($value != '')
			$ret = number_format((float)$value, 2) . '%';
		break;
	case 'SUMMARY.Best Share':
		if ($value != '')
			$ret = number_format((float)$value);
		break;
	}

 if ($section == 'NOTIFY' && substr($name, 0, 1) == '*' && $value != '0')
	$class = $errorclass;

 if ($class == '' && $section != 'POOL')
	$class = classlastshare($when, $alldata, $lstclass, $lstclass);

 if ($class == '' && $section == 'total')
	$class = $totclass;

 if ($class == '' && ($rownum % 2) == 0)
	$class = $c2class;

 if ($ret === '')
	$ret = $b;

 return array($ret, $class);
}
#
global $poolcmd;
$poolcmd = array(	'Switch to'	=> 'switchpool',
			'Enable'	=> 'enablepool',
			'Disable'	=> 'disablepool',
			'Remove'	=> 'removepool' );
#
function showhead($cmd, $values, $justnames = false)
{
 global $poolcmd, $readonly;

 newrow();

 foreach ($values as $name => $value)
 {
	if ($name == '0' or $name == '')
		$name = '&nbsp;';
	echo "<td valign=bottom class=h>$name</td>";
 }

 if ($justnames === false && $cmd == 'pools' && $readonly === false)
	foreach ($poolcmd as $name => $pcmd)
		echo "<td valign=bottom class=h>$name</td>";

 endrow();
}
#
function showdatetime()
{
 global $dfmt;

 otherrow('<td class=sta>Date: '.date($dfmt).'</td>');
}
#
global $singlerigsum;
$singlerigsum = array(
 'devs' => array('MHS av' => 1, 'MHS 5s' => 1, 'Accepted' => 1, 'Rejected' => 1,
			'Hardware Errors' => 1, 'Utility' => 1, 'Total MH' => 1,
			'Diff1 Shares' => 1, 'Diff1 Work' => 1, 'Difficulty Accepted' => 1,
			'Difficulty Rejected' => 1),
 'pools' => array('Getworks' => 1, 'Accepted' => 1, 'Rejected' => 1, 'Discarded' => 1,
			'Stale' => 1, 'Get Failures' => 1, 'Remote Failures' => 1,
			'Diff1 Shares' => 1, 'Diff1 Work' => 1, 'Difficulty Accepted' => 1,
			'Difficulty Rejected' => 1, 'Difficulty Stale' => 1),
 'notify' => array('*' => 1));
#
function showtotal($total, $when, $oldvalues)
{
 global $rigtotals;

 list($showvalue, $class) = fmt('total', '', 'Total:', $when, null);
 echo "<td$class align=right>$showvalue</td>";

 $skipfirst = true;
 foreach ($oldvalues as $name => $value)
 {
	if ($skipfirst === true)
	{
		$skipfirst = false;
		continue;
	}

	if (isset($total[$name]))
		$newvalue = $total[$name];
	else
		$newvalue = '';

	list($showvalue, $class) = fmt('total', $name, $newvalue, $when, null);
	echo "<td$class";
	if ($rigtotals === true)
		echo ' align=right';
	echo ">$showvalue</td>";
 }
}
#
function details($cmd, $list, $rig)
{
 global $dfmt, $poolcmd, $readonly, $showndate;
 global $rownum, $rigtotals, $forcerigtotals, $singlerigsum;

 $when = 0;

 $stas = array('S' => 'Success', 'W' => 'Warning', 'I' => 'Informational', 'E' => 'Error', 'F' => 'Fatal');

 newtable();

 if ($showndate === false)
 {
	showdatetime();

	endtable();
	newtable();

	$showndate = true;
 }

 if (isset($list['STATUS']))
 {
	newrow();
	echo '<td>Computer: '.$list['STATUS']['Description'].'</td>';
	if (isset($list['STATUS']['When']))
	{
		echo '<td>When: '.date($dfmt, $list['STATUS']['When']).'</td>';
		$when = $list['STATUS']['When'];
	}
	$sta = $list['STATUS']['STATUS'];
	echo '<td>Status: '.$stas[$sta].'</td>';
	echo '<td>Message: '.$list['STATUS']['Msg'].'</td>';
	endrow();
 }

 if ($rigtotals === true && isset($singlerigsum[$cmd]))
	$dototal = $singlerigsum[$cmd];
 else
	$dototal = array();

 $total = array();

 $section = '';
 $oldvalues = null;
 foreach ($list as $item => $values)
 {
	if ($item == 'STATUS')
		continue;

	$sectionname = preg_replace('/\d/', '', $item);

	// Handle 'devs' possibly containing >1 table
	if ($sectionname != $section)
	{
		if ($oldvalues != null && count($total) > 0
		&&  ($rownum > 2 || $forcerigtotals === true))
			showtotal($total, $when, $oldvalues);

		endtable();
		newtable();
		showhead($cmd, $values);
		$section = $sectionname;
	}

	newrow();

	foreach ($values as $name => $value)
	{
		list($showvalue, $class) = fmt($section, $name, $value, $when, $values);
		echo "<td$class";
		if ($rigtotals === true)
			echo ' align=right';
		echo ">$showvalue</td>";

		if (isset($dototal[$name])
		||  (isset($dototal['*']) and substr($name, 0, 1) == '*'))
		{
			if (isset($total[$name]))
				$total[$name] += $value;
			else
				$total[$name] = $value;
		}
	}

	if ($cmd == 'pools' && $readonly === false)
	{
		reset($values);
		$pool = current($values);
		foreach ($poolcmd as $name => $pcmd)
		{
			list($ignore, $class) = fmt('BUTTON', 'Pool', '', $when, $values);
			echo "<td$class>";
			if ($pool === false)
				echo '&nbsp;';
			else
			{
				echo "<input type=button value='Pool $pool'";
				echo " onclick='prc(\"$pcmd|$pool&rig=$rig\",\"$name Pool $pool\")'>";
			}
			echo '</td>';
		}
	}
	endrow();

	$oldvalues = $values;
 }

 if ($oldvalues != null && count($total) > 0
 &&  ($rownum > 2 || $forcerigtotals === true))
	showtotal($total, $when, $oldvalues);

 endtable();
}
#
global $devs;
$devs = null;
#
function gpubuttons($count, $rig)
{
 global $devs;

 $basic = array( 'GPU', 'Enable', 'Disable', 'Restart' );

 $options = array(	'intensity' => 'Intensity',
			'fan' => 'Fan Percent',
			'engine' => 'GPU Clock',
			'mem' => 'Memory Clock',
			'vddc' => 'GPU Voltage' );

 newtable();
 newrow();

 foreach ($basic as $head)
	echo "<td class=h>$head</td>";

 foreach ($options as $name => $des)
	echo "<td class=h nowrap>$des</td>";

 $n = 0;
 for ($c = 0; $c < $count; $c++)
 {
	endrow();
	newrow();

	foreach ($basic as $name)
	{
		list($ignore, $class) = fmt('BUTTON', 'GPU', '', 0, null);
		echo "<td$class>";

		if ($name == 'GPU')
			echo $c;
		else
		{
			echo "<input type=button value='$name $c' onclick='prs(\"gpu";
			echo strtolower($name);
			echo "|$c\",$rig)'>";
		}

		echo '</td>';
	}

	foreach ($options as $name => $des)
	{
		list($ignore, $class) = fmt('BUTTON', 'GPU', '', 0, null);
		echo "<td$class>";

		if (!isset($devs["GPU$c"][$des]))
			echo '&nbsp;';
		else
		{
			$value = $devs["GPU$c"][$des];
			echo "<input type=button value='Set $c:' onclick='prs2(\"gpu$name|$c\",$n,$rig)'>";
			echo "<input size=7 type=text name=gi$n value='$value' id=gi$n>";
			$n++;
		}

		echo '</td>';
	}

 }
 endrow();
 endtable();
}
#
function processgpus($rig)
{
 global $error;
 global $warnfont, $warnoff;

 $gpus = api($rig, 'gpucount');

 if ($error != null)
	otherrow("<td>Error getting GPU count: $warnfont$error$warnoff</td>");
 else
 {
	if (!isset($gpus['GPUS']['Count']))
	{
		$rw = '<td>No GPU count returned: '.$warnfont;
		$rw .= $gpus['STATUS']['STATUS'].' '.$gpus['STATUS']['Msg'];
		$rw .= $warnoff.'</td>';
		otherrow($rw);
	}
	else
	{
		$count = $gpus['GPUS']['Count'];
		if ($count == 0)
			otherrow('<td>No GPUs</td>');
		else
			gpubuttons($count, $rig);
	}
 }
}
#
function showpoolinputs($rig, $ans)
{
 global $readonly, $poolinputs;

 if ($readonly === true || $poolinputs === false)
	return;

 newtable();
 newrow();

 $inps = array('Pool URL' => array('purl', 20),
		'Worker Name' => array('pwork', 10),
		'Worker Password' => array('ppass', 10));
 $b = '&nbsp;';

 echo "<td align=right class=h> Add a pool: </td><td>";

 foreach ($inps as $text => $name)
	echo "$text: <input name='".$name[0]."' id='".$name[0]."' value='' type=text size=".$name[1]."> ";

 echo "</td><td align=middle><input type=button value='Add' onclick='pla($rig)'></td>";

 endrow();

 if (count($ans) > 1)
 {
	newrow();

	echo '<td align=right class=h> Set pool priorities: </td>';
	echo "<td> Comma list of pool numbers: <input type=text name=prio id=prio size=20>";
	echo "</td><td align=middle><input type=button value='Set' onclick='psp($rig)'></td>";

	endrow();
 }
 endtable();
}
#
function process($cmds, $rig)
{
 global $error, $devs;
 global $warnfont, $warnoff;

 $count = count($cmds);
 foreach ($cmds as $cmd => $des)
 {
	$process = api($rig, $cmd);

	if ($error != null)
	{
		otherrow("<td colspan=100>Error getting $des: $warnfont$error$warnoff</td>");
		break;
	}
	else
	{
		details($cmd, $process, $rig);

		if ($cmd == 'devs')
			$devs = $process;

		if ($cmd == 'pools')
			showpoolinputs($rig, $process);

		# Not after the last one
		if (--$count > 0)
			otherrow('<td><br><br></td>');
	}
 }
}
#
function rigname($rig, $rigname)
{
 global $rigs;

 if (isset($rigs[$rig]))
 {
	$parts = explode(':', $rigs[$rig], 3);
	if (count($parts) == 3)
		$rigname = $parts[2];
 }

 return $rigname;
}
#
function riginput($rig, $rigname)
{
 $rigname = rigname($rig, $rigname);

 return "<input type=button value='$rigname' onclick='pr(\"&rig=$rig\",null)'>";
}
#
function rigbutton($rig, $rigname, $when, $row)
{
 list($value, $class) = fmt('BUTTON', 'Rig', '', $when, $row);

 if ($rig === '')
	$ri = '&nbsp;';
 else
	$ri = riginput($rig, $rigname);

 return "<td align=middle$class>$ri</td>";
}
#
function showrigs(