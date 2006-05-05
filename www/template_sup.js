/*
 * Some utility stuff.
 */

/* Clear the various 'loading' indicators. */
function ClearLoadingIndicators()
{
    var busyimg = document.getElementById('busy');
    var loadingspan = document.getElementById('loading');

    loadingspan.innerHTML = "";
    busyimg.style.display = "none";
    busyimg.src = "1px.gif";
}

/* Replace the current page */
function PageReplace(URL)
{
    window.location.replace(URL);
}
