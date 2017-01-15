module.exports = {
  entry: "./src/ui.js",
	output: {
		filename: "bundle/tilelite.js"
	},
 	module: {
		loaders: [{
    	test: /\.js$/,
      exclude: /node_modules/,
      loader: "babel-loader",
			query: { presets: ["es2015", "react"] }
		}]
	} 
}
