#include "ts_options.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ini/ini.h"
#include "parg/parg.h"
#include "platform.h"

int32_t compare_case_insensitive(const char* first, size_t len_first,
                                 const char* second, size_t len_second) {
  if (len_first != len_second) return 0;

  for (size_t i = 0; i < len_first; i++) {
    if (tolower(first[i]) != tolower(second[i])) {
      return 0;
    }
  }

  return 1;
}

static const ts_options default_opt = {.plugins = "/usr/local/lib/mapnik/input",
                                       .fonts = "/usr/share/fonts",
                                       .database = "tiles.db",
                                       .host = "127.0.0.1",
                                       .port = "8080",
                                       .rendering = "1",
                                       .mapnik_xml = "osm.xml",
                                       .cache_size = "10M",
                                       .cache_log_factor_str = "10.0",
                                       .cache_decay_seconds_str = "120"};

static int64_t parse_cache_size(const char* cache_string) {
  if (!cache_string) {
    return 0;
  }

  int len = strlen(cache_string);
  const int max_digits = 128;
  int num_digits = 0;
  int suffix_start = -1;
  char digits[max_digits + 1];
  memset(digits, 0, sizeof(digits));

  for (int i = 0; i < strlen(cache_string); i++) {
    char c = cache_string[i];
    if (num_digits < max_digits && (isdigit(c) || c == '.')) {
      digits[num_digits++] = c;
    } else if (isalpha(c)) {
      suffix_start = i;
      break;
    }
  }

  double mul = 1.0;
  if (suffix_start == -1) {
    return 0;
  } else {
    const char* suffix = cache_string + suffix_start;
    int suffix_length = len - suffix_start;
    if (compare_case_insensitive("m", 1, suffix, suffix_length)) {
      mul = 1000000.0;
    } else if (compare_case_insensitive("k", 1, suffix, suffix_length)) {
      mul = 1000.0;
    }
  }

  double base = atof(digits);
  return base * mul;
}

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
               {"mapnik_xml", &opt->mapnik_xml},
               {"cache_size", &opt->cache_size},
               {"cache_log_factor", &opt->cache_log_factor_str},
               {"cache_decay_seconds", &opt->cache_decay_seconds_str}};

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

  opt.cache_size_bytes = parse_cache_size(opt.cache_size);
  opt.cache_log_factor = atof(opt.cache_log_factor_str);
  opt.cache_decay_seconds = atoi(opt.cache_decay_seconds_str);

  return opt;
}
