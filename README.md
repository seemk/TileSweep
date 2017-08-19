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
$ ./bin/tilesweep -c conf.ini
```
## Configuration
```
plugins=/usr/local/lib/mapnik/input/ ; Mapnik plugins directory, requires postgis plugin
fonts=/usr/share/fonts
host=127.0.0.1
port=8080
tile_db=tiles.db
mapnik_xml=openstreetmap-carto/mapnik.xml ; Path to Mapnik's XML stylesheet
rendering=1 ; 1 = Use mapnik, 0 = serve cached tiles
```
