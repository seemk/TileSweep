#pragma once

#include <stdint.h>
#include "common.h"

struct tilelite;
struct image;

bool send_bytes(int fd, const uint8_t* buf, int len);
void send_status(int fd, tl_status status);
void send_server_info(int fd, tilelite* context);
void send_image(int fd, const image* img);
