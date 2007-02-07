<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> Historique de <TMPL_VAR clientname> (<TMPL_VAR Filter>)
  </h1>
 </div>
 <div class='bodydiv'>

   <table id='id<TMPL_VAR ID>'></table>

<a href="bgraph.pl?client=<TMPL_VAR clientname>;action=job_size;status=T">
    <img src="/bweb/chart.png" alt="taille des sauvegardes" title="�volution de la taille des sauvegardes"/>
    </a>
<a href="bgraph.pl?client=<TMPL_VAR clientname>;action=job_duration;status=T">
    <img src="/bweb/chart.png" alt="dur�e des sauvegardes" title="�volution de la dur�e des sauvegardes"/>
    </a>
<a href="bgraph.pl?client=<TMPL_VAR clientname>;action=job_rate;status=T">
    <img src="/bweb/chart.png" alt="vitesse des sauvegardes" title="�volution de la vitesse des sauvegardes"/>
    </a>				
 </div>


<script type="text/javascript" language="JavaScript">
var header = new Array("JobId", "Nom du Job", "File Set", "Niveau", "D�but", 
                  "Nb fichiers", "Taille", "Erreurs");

var data = new Array();

<TMPL_LOOP Jobs>
data.push( new Array(
"<TMPL_VAR JobId>",
"<TMPL_VAR JobName>",    
"<TMPL_VAR FileSet>",
"<TMPL_VAR Level>",
"<TMPL_VAR StartTime>",
"<TMPL_VAR JobFiles>",   
human_size(<TMPL_VAR JobBytes>),
"<TMPL_VAR JobErrors>"   
 )
);
</TMPL_LOOP>

nrsTable.setup(
{
 table_name:     "id<TMPL_VAR ID>",
 table_header: header,
 table_data: data,
 up_icon: up_icon,
 down_icon: down_icon,
 prev_icon: prev_icon,
 next_icon: next_icon,
 rew_icon:  rew_icon,
 fwd_icon:  fwd_icon,
// natural_compare: true,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
 page_nav: true,
 rows_per_page: rows_per_page,
 disable_sorting: new Array(5,6)
}
);

// get newest job first
nrsTables['id<TMPL_VAR ID>'].fieldSort(0);
</script>
