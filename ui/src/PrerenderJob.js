import React from "react";

const PrerenderJob = React.createClass({

  propTypes: {
    job: React.PropTypes.object,
    selected: React.PropTypes.bool,
    onClick: React.PropTypes.func
  },

  render: function() {
    const job = this.props.job;
    
    let classes = "list-group-item ";
    if (this.props.selected) {
      classes += "active ";
    }

    return (
      <div className={classes} onClick={this.props.onClick}>
        <div>
          Zoom: {job.minZoom} - {job.maxZoom}
        </div>
        <div>
          Tiles - {job.numCurrentTiles} / est. {job.numEstimatedTiles}
        </div>
      </div>
    );
  }
});

export default PrerenderJob;
