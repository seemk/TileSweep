import React from "react";
import ReactDOM from "react-dom";
import ol from "openlayers";
import PrerenderJob from "./PrerenderJob.js";
import { startPrerender, loadPrerenderStatus } from "./API.js";

const ControlPanel = React.createClass({

  getInitialState: function() {
    return {
      zoom: 4,
      minZoom: 0,
      maxZoom: 14,
      tileSize256: true,
      tileSize512: false,
      prerenderJobs: [],
      selectedJobId: -1
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
    const that = this;

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

    const submitSuccess = function() {
      that.updateStatus();
    };

    const submitError = function() {
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

  updatePrerenderStatus: function(s) {
    this.setState({
      prerenderJobs: s.renders
    });
  },

  updateStatus: function() {
    loadPrerenderStatus(this.updatePrerenderStatus, (err) => console.log(err));
  },

  componentDidMount: function() {
    const that = this;

    this.osmLayer = new ol.layer.Tile({
      source: new ol.source.OSM()
    });

    this.vectorSource = new ol.source.Vector();

    const vector = new ol.layer.Vector({
      source: this.vectorSource,
      style: f => {
        const defaultStyle = new ol.style.Style({
          fill: new ol.style.Fill({
            color: "rgba(204,204,204,0.2)"
          }),
          stroke: new ol.style.Stroke({
            color: "#525252",
            width: 2
          })
        });

        const s = f.getStyle() || defaultStyle;
        return s;
      }
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
        coordinates: coords,
        selectedJobId: -1
      });
    });

    document.onkeydown = function(evt) {
      // escape
      if (evt.keyCode == 27) {
        that.map.removeInteraction(that.draw);
        that.map.addInteraction(that.draw);
      }
    };

    this.refreshStatusId = setInterval(this.updateStatus, 2000);
    this.updateStatus();
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

  jobClicked: function(job) {
		this.vectorSource.clear();
    if (job.id == this.state.selectedJobId) {
      this.setState({
        selectedJobId: -1
      });
    } else {
      const coordinates = job.boundary;
      const f = new ol.Feature({
        geometry: new ol.geom.Polygon([coordinates]),
      });

      const style = new ol.style.Style({
        stroke: new ol.style.Stroke({
          color: "red",
          width: 2
        }),
        fill: new ol.style.Fill({
          color: "rgba(255, 0, 0, 0.1)"
        })
      });

      f.setStyle(style);

      this.vectorSource.addFeature(f);
      this.setState({
        selectedJobId: job.id
      });
    }
  },

  renderJobList: function() {
    const that = this;
    const activeJobId = this.state.selectedJobId;
    const jobs = this.state.prerenderJobs.map(j =>
      <PrerenderJob
        key={j.id}
        selected={activeJobId == j.id}
        job={j}
        onClick={() => that.jobClicked(j)}
      />
    );

    const filler = "No active render jobs.";
    const content = jobs.length == 0 ? filler : jobs;

    const outerStyle = {
      margin: "12px 2px 2px 2px",
      padding: "2px", 
      width: "100%"
    };

    const listStyle = {
      textAlign: "center",
      width: "100%"
    };

    return (
      <div style={outerStyle}>
        <div style={{textAlign: "center", width: "100%"}}>
          Active render jobs
        </div>
        <div className="list-group" style={listStyle}>
          {content}
        </div>
      </div>
    );
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
          {this.renderJobList()}
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
