<?php
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Search Emulab Documentation");
?>

<center>
<table border=5>
<FORM method=get ACTION="/cgi-bin/webglimpse/usr/testbed/webglimpse">

<tr><td align=center colspan=2 valign=middle>
       <font size=+3>
       <a href="http://glimpse.cs.arizona.edu/webglimpse">WebGlimpse</a>
       Search<br></font>
   </td>
</tr>

<tr>
    <td colspan=2>
        String to search for: <INPUT NAME=query size=30>
        <INPUT TYPE=submit VALUE=Submit>
        <br>

        <INPUT NAME=case TYPE=checkbox>Case&nbsp;sensitive
        <!-- SPACES -->&nbsp;&nbsp;&nbsp;
        <INPUT NAME=whole TYPE=checkbox>Partial&nbsp;match
        <!-- SPACES -->&nbsp;&nbsp;&nbsp;
        <INPUT NAME=lines TYPE=checkbox>Jump&nbsp;to&nbsp;line
	
        <!-- SPACES -->&nbsp;&nbsp;&nbsp;
        <SELECT NAME=errors align=right>
            <OPTION>0
            <OPTION>1
            <OPTION>2
        </SELECT>
        misspellings&nbsp;allowed
	
        <br>
        Return only files modified within the last <INPUT NAME=age size=5>
        days.
	
        <br>
        Maximum number of files returned:
        <SELECT NAME=maxfiles>
            <OPTION>10
            <OPTION selected>50
            <OPTION>100
            <OPTION>1000
        </SELECT>

        <br>
	Maximum number of matches per file returned:
        <SELECT NAME=maxlines>
            <OPTION>10
            <OPTION selected>30
            <OPTION>50
            <OPTION>500
        </SELECT>

        <br>
	Maximum number of characters output per file:
        <INPUT NAME="maxchars" VALUE=10000>
    </td>
</tr>

<tr>
    <td colspan=2>
    <center>
    <font size=-1>
      <a href="http://glimpse.cs.arizona.edu">Glimpse</a> and 
      <a href="http://glimpse.cs.arizona.edu/webglimpse">WebGlimpse</a>, 
      Copyright &copy; 1996, University of Arizona
    </font>
    </center>
    </td>
</tr>
</form>
</table>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

