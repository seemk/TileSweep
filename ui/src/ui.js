import React from "react";
import ReactDOM from "react-dom";
import ol from "openlayers";
import { startPrerender } from "./API.js";

const ControlPanel = React.createClass({

  getInitialState: function() {
    return {
      zoom: 4,
      minZoom: 0,
      maxZoom: 14,
      tileSize256: true,
      tileSize512: false
    }
  },

  minZoomChanged: function(evt) {
    const v = evt.target.value | 0;
    this.setState({
      minZoom: Math.min(this.state.maxZoom, Math.max(0, v))
    });
  },

  maxZoomChanged: function(evt) {
    const v = evt.target.value | 0;
    this.setState({
      maxZoom: Math.min(18, Math.max(this.state.minZoom, v))
    });
  },

  tile256Checked: function(evt) {
    this.setState({
      tileSize256: !this.state.tileSize256
    });
  },

  tile512Checked: function(evt) {
    this.setState({
      tileSize512: !this.state.tileSize512
    });
  },

  submitPrerender: function() {
    console.log("submit prerender");
    if (!this.state.tileSize256 && !this.state.tileSize512) {
      this.setState({
        error: "At least one tile size required."
      });
      return;
    }

    if (!this.state.coordinates) {
      this.setState({
        error: "No area selected."
      });
      return;
    }

    const coordinates = this.state.coordinates;
    const minZoom = this.state.minZoom;
    const maxZoom = this.state.maxZoom;

    var submitSuccess = function() {
      console.log("submit success");
    };

    var submitError = function() {
      console.log("submit error");
    };

    if (this.state.tileSize256) {
      startPrerender({
        minZoom: minZoom,
        maxZoom: maxZoom,
        coordinates: coordinates,
        tileSize: 256
      }, submitSuccess, submitError);
    }

    if (this.state.tileSize512) {
      startPrerender({
        minZoom: minZoom,
        maxZoom: maxZoom,
        coordinates: coordinates,
        tileSize: 512
      }, submitSuccess, submitError);
    }

    this.setState({
      error: null
    });
  },

  componentDidMount: function() {
    const that = this;

    this.osmLayer = new ol.layer.Tile({
      source: new ol.source.OSM()
    });

    this.vectorSource = new ol.source.Vector();

    const vector = new ol.layer.Vector({
      source: this.vectorSource,
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

    this.map = new ol.Map({
      layers: [this.osmLayer, vector],
      target: "map",
      view: new ol.View({
        center: ol.proj.fromLonLat([24.747583, 59.461970]),
        zoom: this.state.zoom
      })
    });

    this.map.on("moveend", () => that.setState({zoom: that.map.getView().getZoom()}));

    this.draw = new ol.interaction.Draw({
      source: this.vectorSource,
      type: "Polygon",
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

    this.map.addInteraction(this.draw);

    this.draw.on("drawend", function(evt) {
      that.vectorSource.clear();
      const coords = evt.feature.getGeometry().getCoordinates()[0].slice(0, -1);
      that.setState({
        coordinates: coords
      });
    });

    document.onkeydown = function(evt) {
      // escape
      if (evt.keyCode == 27) {
        that.map.removeInteraction(that.draw);
        that.map.addInteraction(that.draw);
      }
    };
  },

  renderError: function() {
    if (this.state.error) {
      return (
        <div className="alert alert-danger" role="alert">
          {this.state.error}
        </div>
      );
    }

    return null;
  },
  
  render: function() {
    return (
      <div className="row" style={{padding: "10px 0 10px 0"}}>
        <div className="col-md-2">
          <form>
            <div className="row">
              <label className="col-md-6 col-form-label">Zoom</label>
              <label className="col-md-6 col-form-label">
                {this.state.zoom}
              </label>
            </div>
            <div className="form-group row">
              <label className="col-md-6 col-form-label">Min zoom</label>
              <div className="col-md-6">
                <input
                  className="form-control"
                  type="number"
                  value={this.state.minZoom}
                  onChange={this.minZoomChanged}
                />
              </div>
            </div>
            <div className="form-group row">
              <label className="col-md-6 col-form-label">Max zoom</label>
              <div className="col-md-6">
                <input
                  className="form-control"
                  type="number"
                  value={this.state.maxZoom}
                  onChange={this.maxZoomChanged}
                />
              </div>
            </div>
            <div className="form-check">
              <label className="form-check-label">
                <input
                  className="form-check-input"
                  type="checkbox"
                  checked={this.state.tileSize256}
                  onChange={this.tile256Checked}
                />
                256x256
              </label>
            </div>
            <div className="form-check">
              <label className="form-check-label">
                <input
                  className="form-check-input"
                  type="checkbox"
                  checked={this.state.tileSize512}
                  onChange={this.tile512Checked}
                />
                512x512
              </label>
            </div>
            <button
              type="button"
              onClick={this.submitPrerender}
              className="btn btn-primary">
                Start
            </button>
          </form>
          {this.renderError()}
        </div>
        <div className="col-md-10">
          <div id="map" className="map">
          </div>
        </div>
      </div>
    );
  }

});

ReactDOM.render(<ControlPanel />, document.getElementById("root"));
