# TileSweep

An OpenStreetMap tile server.

Features:
* Prerendering based on polygon and zoom levels selection.
* Offline tile cache (tiles can be prerendered and served without Mapnik)
* Efficient disk usage

## Building
Requirements:
* libmapnik (>= 3.0.12)

```
$ mkdir build && cd build
$ cmake ..
$ make
```
## Usage
```
$ cd bundle
$ ./tilesweep -c conf.ini
```
Open your browser and go to http://localhost:8080 for the control panel.

The tile URL is http://localhost:8080/tile/X/Y/Z/256/256 (or /512/512).

## Configuration
```
plugins=/usr/lib/mapnik/3.0/input/ ; Mapnik plugins directory, requires postgis plugin
fonts=/usr/share/fonts
host=127.0.0.1
port=8080
tile_db=tiles.db
mapnik_xml=/path/to/openstreetmap-carto/mapnik.xml ; Path to Mapnik's XML stylesheet
rendering=1 ; 1 = Use mapnik, 0 = serve cached tiles
```
## Fresh installation

If you don't have OSM data and mapnik.xml yet, refer to https://switch2osm.org/manually-building-a-tile-server-16-04-2-lts/ how to get these. Skip ```Install mod_tile and renderd``` and everything under ```Setting up your webserver```.
