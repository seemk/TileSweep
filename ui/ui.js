function byId(id) { return document.getElementById(id); }

function UI() {

function updateZoomLabel(z) {
  byId("currentZoom").innerHTML = z;
}

function tileURL(tile) {
  return "/tile/" + tile.join("/");
}

function sendPrerenderRequest(prerender, success, fail) {
  var url = "/prerender";
  var req = new XMLHttpRequest();
  req.open("POST", url);
  req.setRequestHeader("Content-Type", "application/json");
  req.onload = function() {
    if (req.status == 200) {
      success(prerender);
    } else {
      fail(prerender);
    }
  };

  req.onerror = function() {
    console.log("failed to send prerender request");  
  };

  req.send(JSON.stringify(prerender));
}

var state = {};
var submitButton;

var raster = new ol.layer.Tile({
  source: new ol.source.OSM()
});

var source = new ol.source.Vector();

var vector = new ol.layer.Vector({
  source: source,
  style: new ol.style.Style({
    fill: new ol.style.Fill({
    color: 'rgba(204,204,204,0.2)'
    }),
    stroke: new ol.style.Stroke({
      color: '#525252',
      width: 2
    })
  })
});

var map = new ol.Map({
  layers: [raster, vector],
  target: 'map',
  view: new ol.View({
    center: ol.proj.fromLonLat([24.747583, 59.461970]),
    zoom: 7
  })
});

updateZoomLabel(map.getView().getZoom());

map.on("moveend", function() {
  updateZoomLabel(map.getView().getZoom());
});

var type = 'Polygon';
var draw = new ol.interaction.Draw({
  source: source,
  type: type,
  style: new ol.style.Style({
    fill: new ol.style.Fill({
      color: 'rgba(204,204,204,0.2)'
    }),
    stroke: new ol.style.Stroke({
      color: '#525252',
      lineDash: [10, 10],
      width: 2
    })
  })
});

map.addInteraction(draw);

draw.on('drawend', function(evt) {
  var coords = evt.feature.getGeometry().getCoordinates()[0].slice(0, -1);
  source.clear();
  console.log(coords);
  state["coordinates"] = coords;
});

function startProgressBar() {
  byId("progressArea").style.visibility = "visible";
}

function finishRender() {
  submitButton.innerHTML = "Start";
}

function tileRenderProgress(done, total) {
  var t = 1.0 - done / total;
  var bar = byId("progress");
  var percentage = Math.min(t * 100.0, 100.0);
  bar.value = percentage;
  byId("progressText").innerHTML = "<span>" + ((total - done) + "/" + total) + "</span>";
}

function showError(text) {
  var err = byId("errorText");
  err.style.visibility = "visible";
  err.innerHTML = text;
}

function hideError() {
  var err = byId("errorText");
  err.style.visibility = "hidden";
  err.innerHTML = "";
}

submitButton = document.getElementById("submit");
submitButton.addEventListener("click", function() {
  var minZoom = parseInt(byId("minZoom").value);
  var maxZoom = parseInt(byId("maxZoom").value);
  var normSize = byId("normSize").checked;
  var retinaSize = byId("retinaSize").checked;
  if (!normSize && !retinaSize) {
    showError("At least one tile size required.");
    return;
  }

  if (!state["coordinates"]) {
    showError("No area selected.");
    return;
  }

  var coordinates = state["coordinates"];

  console.log("Submit pressed");
  console.log("min zoom " + minZoom);
  console.log("max zoom " + maxZoom);
  console.log("coordinates"); console.log(coordinates);

  var submitSuccess = function() {
    console.log("submit success");
  };

  var submitError = function() {
    console.log("submit error");
  };

  var indices = [];
  if (normSize) {
    sendPrerenderRequest({
      minZoom: minZoom,
      maxZoom: maxZoom,
      coordinates: coordinates,
      tileSize: 256
    }, submitSuccess, submitError);
  }

  if (retinaSize) {
    sendPrerenderRequest({
      minZoom: minZoom,
      maxZoom: maxZoom,
      coordinates: coordinates,
      tileSize: 512
    }, submitSuccess, submitError);
  }

  hideError();
});

document.onkeydown = function(evt) {
  // escape
  if (evt.keyCode == 27) {
    map.removeInteraction(draw);
    map.addInteraction(draw);
  }
};

}

(function() {
  if (document.readyState != "loading") {
    UI();
  } else {
    document.addEventListener("DOMContentLoaded", UI);
  }
})();
