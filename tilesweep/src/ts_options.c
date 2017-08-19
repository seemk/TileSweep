#include "ts_options.h"
#include <stdlib.h>
#include <string.h>
#include "ini/ini.h"
#include "parg/parg.h"
#include "platform.h"

static const ts_options default_opt = {.plugins = "/usr/local/lib/mapnik/input",
                                       .fonts = "/usr/share/fonts",
                                       .database = "tiles.db",
                                       .host = "127.0.0.1",
                                       .port = "8080",
                                       .rendering = "1",
                                       .mapnik_xml = "osm.xml"};

static int ini_parse_callback(void* user, const char* section, const char* name,
                              const char* value) {
  ts_options* opt = (ts_options*)user;
  struct {
    const char* name;
    const char** dst;
  } slots[] = {{"plugins", &opt->plugins},
               {"fonts", &opt->fonts},
               {"tile_db", &opt->database},
               {"host", &opt->host},
               {"port", &opt->port},
               {"rendering", &opt->rendering},
               {"mapnik_xml", &opt->mapnik_xml}};

  for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); i++) {
    if (strcmp(name, slots[i].name) == 0) *slots[i].dst = strdup(value);
  }

  return 1;
}

static void usage(void) {
  ts_log("Usage: tilesweep [-c conf_path] [-h]");
  exit(0);
}

ts_options ts_options_parse(int argc, char** argv) {
  ts_options opt = default_opt;

  struct parg_state args;
  parg_init(&args);

  const char* conf_path = NULL;
  int c = -1;
  while ((c = parg_getopt(&args, argc, argv, "c:h")) != -1) {
    switch (c) {
      case 'c':
        conf_path = args.optarg;
        break;
      case 'h':
        usage();
        break;
      case '?':
        if (args.optopt == 'c') {
          ts_log("Option '-c' requires an argument");
          usage();
        }
        break;
      case 1:
      default:
        break;
    }
  }

  if (conf_path == NULL) {
    usage();
  }

  if (ini_parse(conf_path, ini_parse_callback, &opt) < 0) {
    ts_log("failed to load configuration file");
    exit(1);
  }

  return opt;
}
