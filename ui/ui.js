function byId(id) { return document.getElementById(id); }

function UI() {

var TileRequestQueue = function(tiles, onFinish, onProgress, parallel) {
  this.tiles = tiles || [];
  this.initialCount = this.tiles.length;
  this.parallel = parallel || 8;
  this.onFinish = onFinish || function() {};
  this.onProgress = onProgress || function() {};
  this.finishCalled = false;
  this.cancelled = false;
};

TileRequestQueue.prototype.cancel = function() {
  this.cancelled = true;
  this.onFinish();
};

TileRequestQueue.prototype.next = function() {
  if (this.cancelled) {
    return;
  }

  if (this.tiles.length == 0) {
    if (!this.finishCalled) {
      this.finishCalled = true;
      this.onFinish();
    }
    return;
  }

  var tile = this.tiles.pop();
  var req = toRequest(tile, this.next.bind(this), this.onProcess.bind(this));
  req.send();
};

TileRequestQueue.prototype.onProcess = function() {
  this.onProgress(this.tiles.length, this.initialCount);
};

TileRequestQueue.prototype.start = function() {
  var next = this.next.bind(this);
  var prog = this.onProcess.bind(this);
  var tiles = this.tiles.splice(-this.parallel);
  tiles.map(function(t) { return toRequest(t, next, prog); }).forEach(function(r) {
    r.send();
  });
};

var HitTest = function(points) {
  var len = points.length;

  this.points = points;
  this.constants = Array(len);
  this.multiples = Array(len);

  var j = len - 1;
  for (var i = 0; i < len; i++) {
    var pt_i = points[i];
    var pt_j = points[j];
    if (pt_i[1] == pt_j[1]) {
      this.constants[i] = pt_i[0];
      this.multiples[i] = 0.0;
    } else {
      this.constants[i] = pt_i[0] - (pt_i[1] * pt_j[0]) / (pt_j[1] - pt_i[1]) + (pt_i[1] * pt_i[0]) / (pt_j[1] - pt_i[1]);
      this.multiples[i] = (pt_j[0] - pt_i[0]) / (pt_j[1] - pt_i[1]);
    }

    j = i;
  }
};

HitTest.prototype.check = function(pt) {
  var len = this.points.length;
  var j = len - 1;

  var inside = 0;

  for (var i = 0; i < len; i++) {
    var pt_i = this.points[i];
    var pt_j = this.points[j];

    if ((pt_i[1] < pt[1] && pt_j[1] >= pt[1]) ||
        (pt_j[1] < pt[1] && pt_i[1] >= pt[1])) {
      inside ^= (pt[1] * this.multiples[i] + this.constants[i] < pt[0]); 
    }

    j = i;
  }

  return inside > 0;
};

function tileURL(tile) {
  return "/tile/" + tile.join("/");
}

function toRequest(tile, next, progress) {
  var url = tileURL(tile);
  var req = new XMLHttpRequest();
  req.open("GET", url);
  req.onload = function() {
    progress();
    if (req.status < 200 || req.status >= 400) {
      console.log("Failed to request tile, aborting.");
    } else {
      next();
    }
  };
  req.onerror = function() {
    console.log("Failed to render tile: " + url);
    progress();
    next();
  };

  return req;
}

function lonlatToXYZ(lon, lat, z) {
  var latRad = lat * Math.PI / 180.0;
  var n = Math.pow(2.0, z);

  var x = ((lon + 180.0) / 360.0 * n) | 0;
  var y = ((1.0 - Math.log(Math.tan(latRad) + (1.0 / Math.cos(latRad))) / Math.PI) / 2.0 * n) | 0;

  return [x, y, z];
}

function calcIntersections(coords, z, dim) {
  if (coords.length < 3) return [];

  var tlx = Infinity;
  var tly = Infinity;
  var brx = -Infinity;
  var bry = -Infinity;

  for (var i = 0; i < coords.length; i++) {
    var pt = coords[i];
    if (pt[0] < tlx) tlx = pt[0];
    if (pt[1] < tly) tly = pt[1];
    if (pt[0] > brx) brx = pt[0];
    if (pt[1] > bry) bry = pt[1];
  }

  var hitTest = new HitTest(coords);

  var intersections = coords.slice();
  for (var y = tly; y <= bry; y++) {
    for (var x = tlx; x <= brx; x++) {
      var inside = hitTest.check([x, y])
        || hitTest.check([x + 1, y])
        || hitTest.check([x, y + 1])
        || hitTest.check([x + 1, y + 1]);
      if (inside) intersections.push([x, y, z, dim, dim]);
    }
  }

  return intersections;
}

function makePrerenderIndices(coordinates, minZoom, maxZoom, dim) {
  var indices = [];
  for (var z = minZoom; z < maxZoom; z++) {
    var xyzCoords = coordinates
        .map(function(c) { return lonlatToXYZ(c[0], c[1], z); })
        .map(function(c) { c[3] = dim; c[4] = dim; return c; });
    indices = indices.concat(calcIntersections(xyzCoords, z, dim));
  }

  return indices;
}

var state = {
  rendering: false
};
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
  var coords = evt.feature.getGeometry().getCoordinates()[0].map(function(c) {
    return ol.proj.toLonLat(c);
  }).slice(0, -1);
  source.clear();
  state["coordinates"] = coords;
});

function startProgressBar() {
  byId("progressArea").style.visibility = "visible";
}

function finishRender() {
  state["rendering"] = false;
  state["requestPool"] = null;
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
  if (state["rendering"]) {
    state["requestPool"].cancel();
    return;
  }
  var minZoom = byId("minZoom").value;
  var maxZoom = byId("maxZoom").value;
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
  startProgressBar();
  submitButton.innerHTML = "Cancel"; 
  state["rendering"] = true;

  var indices = [];
  if (normSize) {
    var indicesNorm = makePrerenderIndices(coordinates, minZoom, maxZoom, 256);
    indices = indices.concat(indicesNorm);
    console.log("256x256 tiles: " + indicesNorm.length);
  }

  if (retinaSize) {
    var indicesRetina = makePrerenderIndices(coordinates, minZoom, maxZoom, 512);
    indices = indices.concat(indicesRetina);
    console.log("512x512 tiles: " + indicesRetina.length);
  }

  hideError();

  var pool = new TileRequestQueue(indices, finishRender, tileRenderProgress);
  state["requestPool"] = pool;
  pool.start();
});

}

(function() {
  if (document.readyState != "loading") {
    UI();
  } else {
    document.addEventListener("DOMContentLoaded", UI);
  }
})();
