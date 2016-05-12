#include "send.h"
#include "tilelite.h"
#include <sys/socket.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace rj = rapidjson;

bool send_bytes(int fd, const uint8_t* buf, int len) {
  int offset = 0;
  int bytes_left = len;
  while (bytes_left > 0) {
    int bytes_sent = send(fd, buf + offset, bytes_left, 0);

    if (bytes_sent == -1) {
      perror("send to client fail: ");
      return false;
    }

    bytes_left -= bytes_sent;
    offset += bytes_sent;
  }

  return true;
}

void send_status(int fd, tl_status status) {
  rj::StringBuffer buffer;
  rj::Writer<rj::StringBuffer> writer(buffer);

  writer.StartObject();
  writer.Key("status");
  writer.Uint(status);
  writer.EndObject();

  int size = buffer.GetSize() + 1;
  const uint8_t* content = (const uint8_t*)buffer.GetString();

  send_bytes(fd, content, size);
}

void send_server_info(int fd, tilelite* context) {
  rj::StringBuffer buffer;
  rj::Writer<rj::StringBuffer> writer(buffer);
  
  writer.StartObject();
  writer.Key("requestQueueSize");
  writer.Uint(context->pending_requests.size_approx());
  writer.Key("writeQueueSize");
  writer.Uint(context->pending_img_writes.size_approx());
  writer.EndObject();

  int size = buffer.GetSize() + 1;
  const uint8_t* content = (const uint8_t*)buffer.GetString();

  send_bytes(fd, content, size);
}

void send_image(int fd, const image* img) {
  rj::StringBuffer buffer;
  rj::Writer<rj::StringBuffer> writer(buffer);

  writer.StartObject();
  writer.Key("status");
  writer.Uint(tl_ok);
  writer.Key("size");
  writer.Uint(img->len);
  writer.EndObject();

  int size = buffer.GetSize() + 1;
  const uint8_t* content = (const uint8_t*)buffer.GetString();
  send_bytes(fd, content, size);
  send_bytes(fd, img->data, img->len);
}
