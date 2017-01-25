#pragma once

#include <stddef.h>

typedef struct {
	const char* query;
	size_t query_len;
	const char* value;
	size_t value_len;
} query_param_t;

int parse(const char* q, size_t len, query_param_t* params, size_t params_len);
