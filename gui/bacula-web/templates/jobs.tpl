<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" 
  "http://www.w3.org/TR/html4/loose.dtd">
<html lang="en">
<head>
<title>bacula-web</title>
<link rel="stylesheet" type="text/css" href="style/default.css">
{literal}
<script type="text/javascript">
	function OpenWin(URL,wid,hei) {
		window.open(URL,"window1","width="+wid+",height="+hei+",scrollbars=yes,menubar=no,location=no,resizable=no")
	}
</script>
{/literal}

</head>
<body>
{popup_init src='./external_packages/js/overlib.js'}
{include file=header.tpl}

<div id="nav">
  <a href="index.php" title="Back to the dashboard">Dashboard</a> > Jobs list
</div>

<div id="main_center">
  <!-- Failed jobs -->  
  <div class="box">
	<p class="title">Last failed jobs (limited to 10)</p>
	<table class="list">
	  <tr>
		<td class="info">Status</td>
		<td class="info">Job ID</td>
		<td class="info">BackupJob</td>
		<td class="info">Start Time</td>
		<td class="info">End Time</td>
		<td class="info">Elapsed time</td>
		<td class="info">Level</td>
		<td class="info">Pool</td>
	  </tr>
	  {foreach from=$failed_jobs item=job}
	  <tr>
		<td> <img width="20" src="style/images/s_error.gif" alt=""/> </td>
		<td>{$job.JobId}</td>
		<td>{$job.job_name}</td>
		<td>{$job.StartTime}</td>
		<td>{$job.EndTime}</td>
		<td>{$job.elapsed}</td>
		<td align="center">{$job.Level}</td>
		<td>{$job.pool_name}</td>
	  </tr>
	  {/foreach}
	</table>
  </div>
  <!-- Completed jobs --> 
  <div class="box">
	<p class="title">Last completed jobs</p>
	<table class="list">
	  <tr>
		<td class="info">Status</td>
		<td class="info">Job ID</td>
		<td class="info">BackupJob</td>
		<td class="info">Start Time</td>
		<td class="info">End Time</td>
		<td class="info">Elapsed time</td>
		<td class="info">Level</td>
		<td class="info">Pool</td>
	  </tr>
	  {foreach from=$completed_jobs item=job}
	  <tr>
		<td> <img width="20px" src="style/images/s_ok.gif" alt=""/> </td>
		<td>{$job.JobId}</td>
		<td>{$job.job_name}</td>
		<td>{$job.StartTime}</td>
		<td>{$job.EndTime}</td>
		<td>{$job.elapsed}</td>
		<td align="center">{$job.Level}</td>
		<td>{$job.pool_name}</td>
	  </tr>
	  {/foreach}
	</table>	
  </div>
</div>

{include file="footer.tpl"}
