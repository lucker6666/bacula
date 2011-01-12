<?php
  session_start();
  require_once ("paths.php");
  require_once ($smarty_path."Smarty.class.php");
  require_once ("bweb.inc.php");
  require_once ("config.inc.php");  

  $smarty = new Smarty();     
  $dbSql = new Bweb();

  require("lang.php");

  // Smarty configuration
  $smarty->compile_check = true;
  $smarty->debugging = false;
  $smarty->force_compile = true;

  $smarty->template_dir = "./templates";
  $smarty->compile_dir = "./templates_c";
  $smarty->config_dir     = "./configs";
  
  // Global variables
  $job_status = array( 'D' => 'Diff', 'I' => 'Incr', 'F' => 'Full' );

  // Running jobs
  $running_jobs = array();
  
  $query  = "SELECT Job.JobId, Job.JobStatus, Status.JobStatusLong, Job.Name, Job.StartTime, Job.EndTime, Job.Level, Pool.Name AS Pool_name ";
  $query .= "FROM Job ";
  $query .= "JOIN Status ON Job.JobStatus = Status.JobStatus ";
  $query .= "LEFT JOIN Pool ON Job.PoolId = Pool.PoolId ";
  $query .= "WHERE Job.JobStatus IN ('F','S','M','m','s','j','c','d','t','C','R')";
  $query .= "ORDER BY JobId";
  
  $jobsresult = $dbSql->db_link->query( $query );
  
  if( PEAR::isError( $jobsresult ) ) {
	  echo "SQL query = $query <br />";
	  die("Unable to get last failed jobs from catalog" . $jobsresult->getMessage() );
  }else {
	  while( $job = $jobsresult->fetchRow( DB_FETCHMODE_ASSOC ) ) {
	
		// Elapsed time for this job
		$elapsed = 'N/A';
		if( $job['JobStatus'] == 'R' )
			$job['elapsed_time'] = $dbSql->Get_ElapsedTime( strtotime($job['StartTime']), time() );
		else
			$job['elapsed_time'] = 'N/A';
			
		// Odd or even row
		if( count($running_jobs) % 2)
			$job['Job_classe'] = 'odd';

		// Job Status
		$job['Level'] = $job_status[ $job['Level'] ];
		
		array_push( $running_jobs, $job);
	  }
  }
  
  $smarty->assign( 'running_jobs', $running_jobs );
  
  // Get the last jobs list
  $query 	   = "";
  $last_jobs = array();
  
  $query .= "SELECT Job.JobId, Job.Name AS Job_name, Job.StartTime, Job.EndTime, Job.Level, Pool.Name, Job.JobStatus, Pool.Name AS Pool_name, Status.JobStatusLong ";
  $query .= "FROM Job ";
  $query .= "LEFT JOIN Pool ON Job.PoolId=Pool.PoolId ";
  $query .= "LEFT JOIN Status ON Job.JobStatus = Status.JobStatus ";
  
  // Filter by status
  if( isset( $_POST['status'] ) ) {
	switch( $_POST['status'] )
	{
		case 'completed':
			$query .= "WHERE Job.JobStatus = 'T' ";
		break;
		case 'failed':
			$query .= "WHERE Job.JobStatus = 'f' ";
		break;
		case 'canceled':
			$query .= "WHERE Job.JobStatus = 'A' ";
		break;
	}
  }
  
  $query .= "ORDER BY Job.EndTime DESC ";
  
  // Determine how many jobs to display
  if( isset($_POST['limit']) )
	$query .= "LIMIT " . $_POST['limit'];
  else
	$query .= "LIMIT 20 ";
  
  $jobsresult = $dbSql->db_link->query( $query );
  
  if( PEAR::isError( $jobsresult ) ) {
	  echo "SQL query = $query <br />";
	  die("Unable to get last failed jobs from catalog" . $jobsresult->getMessage() );
  }else {
	  while( $job = $jobsresult->fetchRow( DB_FETCHMODE_ASSOC ) ) {
		// Determine icon for job
		if( $job['JobStatus'] == 'T' )
			$job['Job_icon'] = "s_ok.gif";
		else
			$job['Job_icon'] = "s_error.gif";
		
		// Odd or even row
		if( count($last_jobs) % 2)
			$job['Job_classe'] = 'odd';
		
		// Elapsed time for this job
		$job['elapsed_time'] = $dbSql->Get_ElapsedTime( strtotime($job['StartTime']), strtotime($job['EndTime']) );

		// Job Status
                $job['Level'] = $job_status[ $job['Level'] ];

		array_push( $last_jobs, $job);
	  }
  }
  $smarty->assign( 'last_jobs', $last_jobs );
  
  if( isset( $_POST['status'] ) )
	$total_jobs = $dbSql->CountJobs( ALL, $_POST['status'] );
  else
	$total_jobs = $dbSql->CountJobs( ALL );
  
  $smarty->assign( 'total_jobs', $total_jobs );
  
  // Process and display the template 
  $smarty->display('jobs.tpl');
?>
