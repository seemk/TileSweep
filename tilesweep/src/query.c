#include "query.h"
#include <string.h>

int32_t parse_uri_params(const char* q, size_t len, query_param_t* params,
                         size_t params_len) {
  size_t begin_idx = 0;
  for (; begin_idx < len; begin_idx++) {
    if (q[begin_idx] == '?') {
      begin_idx += 1;
      break;
    }
  }

  int parsing_val = 0;

  size_t p = 0;

  memset(params, 0, params_len * sizeof(query_param_t));
  params[p].query = &q[begin_idx];
  for (size_t begin = begin_idx; begin < len; begin++) {
    query_param_t* param = &params[p];
    switch (q[begin]) {
      case '=': {
        parsing_val = 1;
        param->value = q + (begin + 1);
        break;
      };
      case '&': {
        parsing_val = 0;
        p++;
        if (p < params_len) {
          params[p].query = q + (begin + 1);
        } else {
          return p;
        }
        break;
      }
      default: {
        if (parsing_val) {
          param->value_len++;
        } else {
          param->query_len++;
        }
        break;
      }
    }
  }

  return p + 1;
}
