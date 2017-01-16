import React from "react";

const ButtonGroup = React.createClass({

  propTypes: {
    buttons: React.PropTypes.array.isRequired,
    selectedIndex: React.PropTypes.number,
    onClick: React.PropTypes.func
  },

  getDefaultProps: function() {
    return {
      selectedIndex: -1,
      onClick: function() {}
    }
  },

  render: function() {
    const onClick = this.props.onClick;
    const selected = this.props.selectedIndex;
    const buttons = this.props.buttons.map((b, i) => {
      const checked = i == selected;
      let classes = "btn btn-primary";

      if (checked) {
        classes += " active";
      }

      return (
        <button
          type="button"
          key={i}
          className={classes}
          onClick={() => onClick(b, i)}>
          {b.name}
        </button>
      );
    });

    return (
      <div className="btn-group" role="group">
        {buttons}
      </div>
    );
  }
});

export default ButtonGroup;
