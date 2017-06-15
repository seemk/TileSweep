import React from "react";
import { Label, Segment, Progress, List } from "semantic-ui-react";

export default class PrerenderJob extends React.Component {

  render() {
    let job = this.props.job;
    let percent = Math.min(job.numCurrentTiles / job.numEstimatedTiles * 100.0, 100.0);
    return (
      <List.Item onClick={this.props.onClick} active={this.props.selected}>
        <Segment>
          <Label>Zoom {job.minZoom} - {job.maxZoom}</Label>
          <Label>
            {job.numCurrentTiles} of {job.numEstimatedTiles} (estimated) tiles
          </Label>
          <Progress percent={percent} color="green" active size="small" attached="bottom" />
        </Segment>
      </List.Item>
    );
  }

}
