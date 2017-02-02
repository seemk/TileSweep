#include <h2o.h>
#include "../query.h"
#include "minunit.h"

int32_t check_parameter(const query_param_t* p, const char* query,
                        const char* value) {
  return h2o_memis(p->query, p->query_len, query, strlen(query)) &&
         h2o_memis(p->value, p->value_len, value, strlen(value));
}

static const char* test_valid_query() {
  const char* q = "https://www.foobar.com?p1=42&p2=foobar&p3=111";

  query_param_t params[3];

  int32_t n_parsed = parse_uri_params(q, strlen(q), params, 3);
  mu_assert("parsed parameter count", n_parsed == 3);
  mu_assert("parse query p1", check_parameter(&params[0], "p1", "42"));
  mu_assert("parse query p2", check_parameter(&params[1], "p2", "foobar"));
  mu_assert("parse query p3", check_parameter(&params[2], "p3", "111"));

  return NULL;
}

static const char* test_query_parse() {
  mu_run_test(test_valid_query);
  return NULL;
}
