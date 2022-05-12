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
			'STATS.Times Rec