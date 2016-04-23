#include "miniunit.h"
#include "../tl_message.h"
#include <string.h>
#include <stdio.h>

const char* empty_string() {
  const uint8_t empty[] = {'\0'};
  tl_message m;
  mu_assert("empty message", tlm_start(empty, sizeof(empty), &m) == 0);
  return NULL;
}

const char* short_string() {
  const uint8_t short_msg[] = {'b', '\0'};
  tl_message m;
  mu_assert("short message", tlm_start(short_msg, sizeof(short_msg), &m) == 0);
  return NULL;
}

const char* i32_string() {
  const uint8_t i32_msg[] = {'i', '\0', 0x7, 0x5b, 0xcd, 0x15};
  tl_message m;
  mu_assert("i32 message", tlm_start(i32_msg, sizeof(i32_msg), &m) == 1);
  mu_assert("i32 next", tlm_next(&m) == tlm_i32);
  mu_assert("i32 next end", tlm_next(&m) == tlm_none);
  mu_assert("i32 read", tlm_read_i32(&m) == 123456789);
  return NULL;
}

const char* i32_multi() {
  // 123456789, 0, 291121006, 598438445
  const uint8_t msg[] = {'i',  'i',  'i',  'i',  '\0', 0x7,  0x5b,
                         0xcd, 0x15, 0x00, 0x00, 0x00, 0x00, 0x11,
                         0x5a, 0x27, 0x6e, 0x23, 0xab, 0x72, 0x2d};
  tl_message m;
  mu_assert("i32 message", tlm_start(msg, sizeof(msg), &m) == 1);
  mu_assert("i32 next", tlm_next(&m) == tlm_i32);
  mu_assert("i32 read", tlm_read_i32(&m) == 123456789);
  mu_assert("i32 next", tlm_next(&m) == tlm_i32);
  mu_assert("i32 read", tlm_read_i32(&m) == 0);
  mu_assert("i32 next", tlm_next(&m) == tlm_i32);
  mu_assert("i32 read", tlm_read_i32(&m) == 291121006);
  mu_assert("i32 next", tlm_next(&m) == tlm_i32);
  mu_assert("i32 read", tlm_read_i32(&m) == 598438445);
  mu_assert("i32 next end", tlm_next(&m) == tlm_none);
  mu_assert("i32 next x2 end", tlm_next(&m) == tlm_none);
  return NULL;
}

const char* byte_read() {
  const uint8_t msg[] = {'b', 'b', 'b', '\0', 0x2, 0x4, 0x8 };
  tl_message m;
  mu_assert("byte read start", tlm_start(msg, sizeof(msg), &m) == 1);
  mu_assert("byte next", tlm_next(&m) == tlm_byte);
  mu_assert("byte read", tlm_read_byte(&m) == 0x2);
  mu_assert("byte next", tlm_next(&m) == tlm_byte);
  mu_assert("byte read", tlm_read_byte(&m) == 0x4);
  mu_assert("byte next", tlm_next(&m) == tlm_byte);
  mu_assert("byte read", tlm_read_byte(&m) == 0x8);
  mu_assert("byte next end", tlm_next(&m) == tlm_none);
  return NULL;
};

static const char* tests() {
  mu_run_test(empty_string);
  mu_run_test(short_string);
  mu_run_test(i32_string);
  mu_run_test(i32_multi);
  mu_run_test(byte_read);
  return NULL;
}

int main(int argc, char** argv) {
  const char* res = tests();
  if (res != 0) {
    printf("failed: %s\n", res);
  }
  return res != 0;
}
