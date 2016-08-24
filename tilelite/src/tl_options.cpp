#include "tl_options.h"
#include "ini/ini.h"
#include "parg/parg.h"
#include "tl_log.h"

static void set_defaults(tl_options* conf) {
  auto set_key = [conf](const char* key, const char* value) {
    if (conf->count(key) == 0) (*conf)[key] = value;
  };

  set_key("plugins", "");
  set_key("fonts", "");
  set_key("tile_db", "tiles.db");
  set_key("host", "127.0.0.1");
  set_key("port", "9567");
  set_key("rendering", "1");
}

int ini_parse_callback(void* user, const char* section, const char* name,
                       const char* value) {
  tl_options* conf = (tl_options*)user;
  (*conf)[name] = value;

  return 1;
}

tl_options parse_options(int argc, char** argv) {
  tl_options conf;

  parg_state args;
  parg_init(&args);

  auto usage = []() {
    tl_log("Usage: tilelite [-c conf_path] [-h]");
    exit(0);
  };

  int c = -1;
  while ((c = parg_getopt(&args, argc, argv, "c:h")) != -1) {
    switch (c) {
      case 'c':
        conf["conf_file_path"] = std::string(args.optarg);
        break;
      case 'h':
        usage();
        break;
      case '?':
        if (args.optopt == 'c') {
          tl_log("Option '-c' requires an argument");
          usage();
        }
        break;
      case 1:
      default:
        break;
    }
  }

  if (conf.count("conf_file_path") == 0) {
    usage();
  }

  if (ini_parse(conf["conf_file_path"].c_str(), ini_parse_callback, &conf) <
      0) {
    tl_log("failed to load configuration file");
    exit(1);
  }

  set_defaults(&conf);

  return conf;
}
