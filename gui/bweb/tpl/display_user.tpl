<div class='titlediv'>
  <h1 class='newstitle'>__User:__ <TMPL_VAR username></h1>
</div>
<div class='bodydiv'>

<form name="form1" action="?">
<input type="hidden" value='1' name="create">
 <table>
 <tr>
  <td>__Username:__</td> <td> <input class="formulaire" type="text" name="username" value='<TMPL_VAR username>'> </td>
<!-- </tr><tr>
  <td>__Password:__</td> <td> <input class="formulaire" type="password" name="passwd" value='<TMPL_VAR passwd>'> </td>
-->
 </tr><tr>
  <td>__Comment:__</td> <td> <input class="formulaire" type="text" name="comment" value='<TMPL_VAR comment>'> </td>
 </tr><tr>
  <td>__Language:__</td> 
   <td> 
 <select name="lang" id='lang' class="formulaire">
  <option id='lang_en' value='en'>__English__</option>
  <option id='lang_fr' value='fr'>__French__</option>
  <option id='lang_es' value='es'>__Spanish__</option>
 </select>
   </td>
 </tr><tr>
<td> __Profile:__</td><td>
 <select name="profile" id='profile' class="formulaire">
  <option onclick='set_role("")'></option>
  <option onclick='set_role("administrator")'>__Administrator__</option>
  <option onclick='set_role("customer")'>__Customer__</option>
 </select>
</td><td>__Or like an existing user:__ </td><td>
 <select name="copy_username" class="formulaire">
  <option onclick="disable_sel(false)"></option>
 <TMPL_LOOP db_usernames>
  <option title="<TMPL_VAR comment>" onclick="disable_sel(true)" value='<TMPL_VAR username>'><TMPL_VAR username></option>
 </TMPL_LOOP>
 </select>
</td>
 </tr><tr>
 </tr><tr>
<td> __Roles:__</td><td>
 <select name="rolename" id='rolename' multiple class="formulaire" size=15>
 <TMPL_LOOP db_roles>
  <option title="<TMPL_VAR comment>" value='<TMPL_VAR rolename>' <TMPL_IF userid>selected</TMPL_IF> ><TMPL_VAR rolename></option>
 </TMPL_LOOP>
 </select>
 </td>
</tr><tr>
<td> __Use groups filter:__</td><td>
<input class="formulaire" onclick="disable_group(this.checked == false)" type="checkbox" name="use_acl" <TMPL_IF use_acl> checked </TMPL_IF> > </td>
</tr><tr>
<td> __Groups:__</td><td>
 <select name="client_group" id='client_group' multiple class="formulaire" size=15>
<TMPL_LOOP db_client_groups>
        <option id='group_<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
 </select>
 </td>
</tr>
</table>
    <button type="submit" class="bp" name='action' value='user_save'>
     <img src='/bweb/save.png' alt=''> </button>
</form>
</div>

<script type="text/javascript" language='JavaScript'>

<TMPL_LOOP client_group>
    document.getElementById('group_<TMPL_VAR name>').selected = true;
</TMPL_LOOP>

<TMPL_UNLESS use_acl>
disable_group(true); 
</TMPL_UNLESS>

function disable_sel(val) 
{
	document.form1.profile.disabled = val;
	document.form1.rolename.disabled = val;
}
function disable_group(val) 
{
	document.form1.client_group.disabled = val;
}
function set_role(val)
{
   if (val == "administrator") {
	for (var i=0; i < document.form1.rolename.length; ++i) {
	      document.form1.rolename[i].selected = true;
	}
   } else if (val == "production") {
	for (var i=0; i < document.form1.rolename.length; ++i) {
	   if (document.form1.rolename[i].value != 'r_configure' &&
               document.form1.rolename[i].value != 'r_user_mgnt' &&
               document.form1.rolename[i].value != 'r_group_mgnt'
               )
           {
	      document.form1.rolename[i].selected = true;
           }
        }
   } else if (val == "customer") {
	for (var i=0; i < document.form1.rolename.length; ++i) {
	   if (document.form1.rolename[i].value == 'r_view_stats'   ||
               document.form1.rolename[i].value == 'r_view_history' ||
               document.form1.rolename[i].value == 'r_view_log'
               )
           {
	      document.form1.rolename[i].selected = true;
	   } else {
	      document.form1.rolename[i].selected = false;
	   }
	}
   }

}
<TMPL_IF lang>
  document.getElementById('lang_<TMPL_VAR lang>').selected = true;
</TMPL_IF>
</script>
