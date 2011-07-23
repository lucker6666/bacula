<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" 
  "http://www.w3.org/TR/html4/loose.dtd">
<html lang="en">
<head>
<title>bacula-web</title>
<link rel="stylesheet" type="text/css" href="style/default.css">
</head>
<body>
{include file=header.tpl}

  <div id="nav">
    <a href="index.php" title="Back to the dashboard">Dashboard</a> > Backup Job Report
  </div>

  <div id="main_center">
  
  <div class="box">
	<p class="title">Backup Job Report</p>
	
	<table>
		<tr>
			<td width="150">Backup Job name:</td>
			<td>{$backupjob_name}</td>
		</tr>
		<tr>
			<td>Period:</td>
			<td>{$backupjob_period}</td>
		</tr>
		<tr>
			<td>Transfered Bytes</td>
			<td>{$backupjob_bytes}</td>
		</tr>
		<tr>
			<td>Transfered Files</td>
			<td>{$backupjob_files}</td>
		</tr>

	</table>
  </div> <!-- end div class=box -->
  
  <!-- Last jobs list -->
  <div class="box">
	<p class="title">Last jobs</p>
	
	<table>
		<tr>
			<td class="tbl_header" width="60">Job Id</td>
			<td class="tbl_header" width="60">Level</td>
			<td class="tbl_header" width="70">Files</td>
			<td class="tbl_header" width="70">Bytes</td>
			<td class="tbl_header" width="80">Start time</td>
			<td class="tbl_header" width="80">End time</td>
			<td class="tbl_header" width="80">Elapsed time</td>
		</tr>
		{foreach from=$jobs item=job}
		<tr>
			<td class="{$job.row_class}">{$job.jobid}</td>
			<td class="{$job.row_class}">{$job.joblevel}</td>
			<td class="{$job.row_class}">{$job.jobfiles}</td>
			<td class="{$job.row_class}">{$job.jobbytes}</td>
			<td class="{$job.row_class}">{$job.starttime}</td>
			<td class="{$job.row_class}">{$job.endtime}</td>
			<td class="{$job.row_class}">{$job.elapsedtime}</td>
		</tr>
		{/foreach}
	</table>
  </div> <!-- end div class=box -->
  
  <!-- Transfered Bytes/Files graph -->
  <div class="box">
	<p class="title">Transfered Bytes / Files</p>
	<img class="graph" src="{$graph_stored_bytes}" alt="" />
	<img class="graph" src="{$graph_stored_files}" alt="" />
  </div> <!-- end div class=box -->
  
  </div> <!-- end div id=main_center -->

{include file="footer.tpl"}
