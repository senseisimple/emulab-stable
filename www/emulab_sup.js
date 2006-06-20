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
