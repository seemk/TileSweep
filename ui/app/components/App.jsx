import React from "react";
import ReactDOM from "react-dom";
import ol from "openlayers";
import PrerenderJob from "./PrerenderJob.jsx";
import { startPrerender, loadPrerenderStatus } from "../API.js";
import { List, Sidebar, Segment, Button, Form, Header, Message, Container } from "semantic-ui-react";

function isEmpty(v) {
  return v == undefined || v == null;
}

const RenderList = (props) => {
  let {jobs, activeJobId, jobClicked} = props;
  let jobList = jobs.map((j, i) => {
    let selected = activeJobId == j.id;
    return (
      <PrerenderJob job={j} key={i}
        selected={selected}
        onClick={() => jobClicked(j)}
      />
    );
  });
  return (
    <List selection>
      {jobList}
    </List>
  );
};

export default class App extends React.Component {

  constructor(props, ctx) {
    super(props, ctx);

    this.state = {
      zoom: 4,
      minZoom: 0,
      maxZoom: 14,
      tileSize256: true,
      tileSize512: false,
      prerenderJobs: [],
      selectedJobId: -1,
      tileMode: "osm"
    };
  }

  minZoomChanged(evt) {
    const v = evt.target.value | 0;
    this.setState({
      minZoom: Math.min(this.state.maxZoom, Math.max(0, v))
    });
  }

  maxZoomChanged(evt) {
    const v = evt.target.value | 0;
    this.setState({
      maxZoom: Math.min(18, Math.max(this.state.minZoom, v))
    });
  }

  tile256Checked() {
    this.setState({
      tileSize256: !this.state.tileSize256
    });
  }

  tile512Checked() {
    this.setState({
      tileSize512: !this.state.tileSize512
    });
  }

  submitPrerender(e) {
    e.preventDefault();
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
      that.vectorSource.clear();
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
  }

  updatePrerenderStatus(s) {
    this.setState({
      prerenderJobs: s.renders || []
    });
  }

  updateStatus() {
    let onFinish = this.updatePrerenderStatus.bind(this);
    loadPrerenderStatus(onFinish, (err) => console.log(err));
  }

  componentDidMount() {
    const that = this;

    this.cachedSource = new ol.source.XYZ({
      url: "/tile/{x}/{y}/{z}/256/256?cached=true"
    });

    this.cachedLayer = new ol.layer.Tile({
      source: this.cachedSource
    });

    this.osmLayer = new ol.layer.Tile({
      source: new ol.source.OSM()
    });

    this.vectorSource = new ol.source.Vector();

    this.vectorLayer = new ol.layer.Vector({
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
      layers: [this.osmLayer, this.vectorLayer],
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

    this.refreshStatusId = setInterval(this.updateStatus.bind(this), 2000);
    this.updateStatus();
  }

  jobClicked(job) {
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
  }

  setTileMode(mode) {
    const map = this.map;
    const layers = [this.osmLayer, this.vectorLayer, this.cachedLayer];

    layers.forEach(l => map.removeLayer(l));

    if (mode == "osm") {
      map.addLayer(this.osmLayer);
      map.addLayer(this.vectorLayer);
      
      if (this.cacheSourceRefreshTimer) {
        clearInterval(this.cacheSourceRefreshTimer);
      }
    } else {
      map.addLayer(this.cachedLayer);
      map.addLayer(this.vectorLayer);
      const source = this.cachedSource;
      source.refresh();
      this.cacheSourceRefreshTimer = setInterval(() => source.refresh(), 5000);
    }

    this.setState({
      tileMode: mode
    });
  }

  render() {
    let that = this;
    let state = this.state;

    return (
      <Sidebar.Pushable as={Segment} style={{height: "100%", width: "100%"}}>
        <Sidebar as={Segment} visible={true} width={"very wide"} padded={true}>
          <Button.Group fluid>
            <Button positive={state.tileMode == "osm"}
              onClick={() => that.setTileMode("osm")}>
              Online tiles
            </Button>
            <Button.Or />
            <Button positive={state.tileMode == "cached"}
              onClick={() => that.setTileMode("cached")}>
              Cached tiles
            </Button>
          </Button.Group> 
          <Header size="tiny">Current zoom {state.zoom.toFixed(1)}</Header>
          <Form onSubmit={this.submitPrerender.bind(this)} error={!isEmpty(state.error)}>
            <Form.Field>
              <label>Min zoom</label>
              <input type="number"
                value={state.minZoom}
                onChange={this.minZoomChanged.bind(this)}
              />
            </Form.Field>
            <Form.Field>
              <label>Max zoom</label>
              <input type="number"
                value={state.maxZoom}
                onChange={this.maxZoomChanged.bind(this)}
              />
            </Form.Field>
            <Form.Group inline>
              <label>Size</label>
              <Form.Checkbox label="256x256"
                checked={state.tileSize256}
                onChange={this.tile256Checked.bind(this)}
              />
              <Form.Checkbox label="512x512"
                checked={state.tileSize512}
                onChange={this.tile512Checked.bind(this)}
              />
            </Form.Group>
            <Message error content={state.error} />
            <Form.Button>
              Start
            </Form.Button>
          </Form>
          <Segment>
            <Header textAlign="center" size="tiny">Ongoing jobs</Header>
            <RenderList jobs={state.prerenderJobs}
              activeJobId={state.selectedJobId}
              jobClicked={this.jobClicked.bind(this)}
            />
          </Segment>
        </Sidebar>
        <Sidebar.Pusher as={Segment} padded={false} style={{padding: "0", width: "100%", height: "100%"}}>
          <div id="map" className="map" style={{height: "100%"}}>
          </div>
        </Sidebar.Pusher>
      </Sidebar.Pushable>
    );
  }

}
