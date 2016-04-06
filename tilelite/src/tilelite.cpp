#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "image_db.h"
#include <string>
#include "tile_renderer.h"
#include "ini/ini.h"
#include "tile.h"
#include "image.h"

int64_t position_hash(int64_t z, int64_t x, int64_t y) {
  return (z << 40) | (x << 20) | y;
}

int atoi_len(const char* s, int len) {
  int n = 0;

  for (int i = len; i > 0; i--) {
    n = n * 10 + (*s++ - '0');
  }

  return n;
}

tile parse_tile(const char* s, int len) {
  int vals[5] = {0};
  char buf[16];
  int idx = 0;
  int v_idx = 0;

  for (int i = 0; i < len; i++) {
    char c = s[i];
    if (c == ',') {
      buf[idx] = '\0';
      vals[v_idx++] = atoi(buf);
      idx = 0;

      if (v_idx == 4) {
        // parse remaining after last comma
        for (int j = i + 1; j < len; j++) {
          buf[idx++] = s[j];
        }
        buf[idx] = '\0';
        vals[v_idx] = atoi(buf);
        break;
      }
    } else {
      buf[idx++] = c;
    }
  }

  tile c;
  memcpy(&c, vals, sizeof(vals));
  return c;
}

struct tile_request {
  struct sockaddr_storage remote_addr;
  socklen_t remote_addr_len;
  tile tile_dim;
};

struct tilelite {
  tile_renderer renderer;
};

int ini_parse_callback(void* user, const char* section, const char* name,
                       const char* value) {
  tilelite* context = (tilelite*)user;

  if (strcmp(name, "plugins") == 0) {
    register_plugins(value);
  } else if (strcmp(name, "fonts") == 0) {
    register_fonts(value);
  } else if (strcmp(name, "mapnik_xml") == 0) {
    tile_renderer_init(&context->renderer, value);
  }

  return 1;
}

int main(int argc, char** argv) {
  tilelite context;

  if (ini_parse("conf.ini", ini_parse_callback, &context) < 0 ||
      context.renderer.map == NULL) {
    fprintf(stderr, "failed load configuration file\n");
    return 1;
  }

  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage remote_addr;
  char buf[1024];

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  int rv;
  if ((rv = getaddrinfo(NULL, "9567", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  int sockfd;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      printf("socket fail\n");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      printf("bind fail\n");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "failed to bind socket");
    return 1;
  }

  freeaddrinfo(servinfo);

  image_db* db = image_db_open("image.db");

  if (!db) {
    fprintf(stderr, "failed to open DB\n");
    return 1;
  }

  socklen_t addr_len = sizeof(remote_addr);

  for (;;) {
    int numbytes = recvfrom(sockfd, buf, 1023, 0,
                            (struct sockaddr*)&remote_addr, &addr_len);

    buf[numbytes] = '\0';

    printf("%s\n", buf);

    tile coord = parse_tile(buf, numbytes);
    printf("%d %d %d %d %d\n", coord.z, coord.x, coord.y, coord.w, coord.h);

    int64_t pos_hash = position_hash(coord.z, coord.x, coord.y);
    image img;
    img.data = NULL;
    int fetch_res = image_db_fetch(db, pos_hash, &img);

    if (fetch_res == -1) {
      const int MTU_MAX = 512;
      render_tile(&context.renderer, &coord, &img);
      const int bytes_to_send = img.len;

      int bytes_sent = sendto(sockfd, img.data, MTU_MAX, 0,
                              (const struct sockaddr*)&remote_addr, addr_len);
      printf("sent %d bytes\n", bytes_sent);
    }

    if (img.data) {
      free(img.data);
    }
  }

  close(sockfd);

  image_db_close(db);

  return 0;
}
