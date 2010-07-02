var map = null;
var markers = [];
var counts = [];
var markerClusterer = null;
var individual = false;

/*
 * Set up the map - should be called as "onload" on the body element
 */
function usermap_initialize() {
    if (GBrowserIsCompatible()) {
        // Grab the element to display the map in - should be a div with ID
        // "map_canvas"
        map = new GMap2(document.getElementById("map_canvas"));

        // Pick a center point and zoom level that fits most markers on
        map.setCenter(new GLatLng(25, 5), 2);

        // Use the default control set from maps.google.com
        map.setUIToDefault();

        // Use the default red map icon when displaying individual points
        var icon = new GIcon(G_DEFAULT_ICON);

        // The JSON file with city points should have been loaded by now,
        // which sets up the 'users' variable. Put all points from it into
        // markers
        for (var i = 0; i < users.cities.length; ++i) {
            var latlng = new GLatLng(users.cities[i].latitude,
                                     users.cities[i].longitude);
            var marker = new GMarker(latlng,{'icon': icon});
            marker.count = users.cities[i].count;
            marker.name = users.cities[i].name;

            // Set the text that pops up if the user clicks on the marker
            marker.bindInfoWindowHtml(marker.name + " <i>(" +
                                      marker.count + ")</i>");

            markers.push(marker);
        }

        // Add a custom control, defined below, to view specific regions, etc
        map.addControl(new ViewSetControl());

        // Start the map up!
        refreshMap();
    }
}

/*
 * Display icons, clusters, etc. on the map
 */
function refreshMap() {
    /*
     * Clear out any old state
     */
    if (markerClusterer != null) {
      markerClusterer.clearMarkers();
    }
    // If we've put any individual markers on the map, this clears them off
    map.clearOverlays();

    /*
     * Put new points on the map
     */
    if (individual) {
        // If displaying individual cities, rather than clusters, just add
        // the markers directly to the map
        for (var i = 0; i < markers.length; i++) {
            map.addOverlay(markers[i]);
        }
    } else {
        // If doing clusters, the MarkerClusterer object does all the work
        markerClusterer = new MarkerClusterer(map, markers,
            {'maxZoom': 20, 'gridSize': 20, 'styles': null});
    }
}

/*
 * Custom control objects
 */

// We define the function first
function ViewSetControl() {
}

// To "subclass" the GControl, we set the prototype object to
// an instance of the GControl object
ViewSetControl.prototype = new GControl();

// Creates a SPAN for each of the buttons and places them in a container
// DIV which is returned as our control element. We add the control to
// to the map container and return the element for the map class to
// position properly.
ViewSetControl.prototype.initialize = function(map) {
  var container = document.createElement("div");
  this.setContainerStyle_(container);

  // Button to show whole world
  var worldspan = document.createElement("span");
  this.setButtonStyle_(worldspan);
  worldspan.style.borderLeft = "";
  container.appendChild(worldspan);
  worldspan.appendChild(document.createTextNode("Worldwide"));
  GEvent.addDomListener(worldspan, "click", function() {
    map.setCenter(new GLatLng(25, 5), 2);
  });

  // Button to show just the USA
  var USAspan = document.createElement("span");
  this.setButtonStyle_(USAspan);
  container.appendChild(USAspan);
  USAspan.appendChild(document.createTextNode("USA"));
  GEvent.addDomListener(USAspan, "click", function() {
    map.setCenter(new GLatLng(37, -100), 4);
  });

  // Button to show just Europe
  var europespan = document.createElement("span");
  this.setButtonStyle_(europespan);
  container.appendChild(europespan);
  europespan.appendChild(document.createTextNode("Europe"));
  GEvent.addDomListener(europespan, "click", function() {
    map.setCenter(new GLatLng(48, 15), 4);
  });

  // Button to toggle between individual points and clusters
  var togglespan = document.createElement("span");
  this.setButtonStyle_(togglespan);
  container.appendChild(togglespan);
  var toggleTextOff = document.createTextNode("Show all markers");
  var toggleTextOn = document.createTextNode("Show clusters");
  togglespan.appendChild(toggleTextOff);
  GEvent.addDomListener(togglespan, "click", function() {
    // Toggle the button text back and forth
    if (individual) {
      individual = false;
      togglespan.replaceChild(toggleTextOff,toggleTextOn);
    } else {
      togglespan.replaceChild(toggleTextOn,toggleTextOff);
      individual = true;
    }
    // Redraw the map
    refreshMap();
  });

  map.getContainer().appendChild(container);
  return container;
}

// Put the control in the bottom right corner with enough room for the google
// copyright text below it.
ViewSetControl.prototype.getDefaultPosition = function() {
  return new GControlPosition(G_ANCHOR_BOTTOM_RIGHT, new GSize(25, 15));
}

// Sets the proper CSS for the container element
ViewSetControl.prototype.setContainerStyle_ = function(container) {
  container.style.color = "#0000cc";
  container.style.backgroundColor = "white";
  container.style.font = "small Arial";
  container.style.border = "1px solid black";
  container.style.padding = "0px";
  container.style.textAlign = "center";
  container.style.cursor = "pointer";
  container.style.float = "left";
}

// Sets the proper CSS for the given button
ViewSetControl.prototype.setButtonStyle_ = function(button) {
    button.style.borderLeft = "1px solid black";
    button.style.paddingRight = "7px";
    button.style.paddingLeft = "7px";
    button.style.paddingRight = "3px";
    button.style.paddingLeft = "3px";
    button.style.margin = "0px";
}
