/*
 * Some utility stuff.
 */

/* Clear the various 'loading' indicators. */
function ClearLoadingIndicators(done_msg)
{
    var busyimg = document.getElementById('busy');
    var loadingspan = document.getElementById('loading');

    loadingspan.innerHTML = done_msg;
    busyimg.style.display = "none";
    busyimg.src = "1px.gif";
}

function ClearBusyIndicators(done_msg)
{
	ClearLoadingIndicators(done_msg);
}

/* Replace the current page */
function PageReplace(URL)
{
    window.location.replace(URL);
}

function IframeDocument(id) 
{
    var oIframe = document.getElementById(id);
    var oDoc    = (oIframe.contentWindow || oIframe.contentDocument);

    if (oDoc.document) {
	oDoc = oDoc.document;
    }
    return oDoc;
}

function GraphChange_cb(html) {
    grapharea = getObjbyName('grapharea');
    if (grapharea) {
        grapharea.innerHTML = html;
    }
}

function GraphChange(which) {
    var arg0 = "all";
    var arg1 = "all";
    
    var srcvnode = getObjbyName('trace_srcvnode');
    if (srcvnode) {
	arg0 = srcvnode.options[srcvnode.selectedIndex].value;
    }
    var dstvnode = getObjbyName('trace_dstvnode');
    if (dstvnode) {
	arg1 = dstvnode.options[dstvnode.selectedIndex].value;
    }

    x_GraphShow(which, arg0, arg1, GraphChange_cb);
    return false;
}
