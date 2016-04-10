local bit = require("bit")

local sock = ngx.socket.tcp();
sock:settimeout(60000)
local ok, err = sock:connect("127.0.0.1", 9567)
if not ok then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

local w = ngx.var.w or 256
local h = ngx.var.h or 256

query = string.format("%d,%d,%d,%d,%d", ngx.var.z, ngx.var.x, ngx.var.y, w, h)
ok, err = sock:send(query)

local size_data, err = sock:receive(4)
if not size_data then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

local b1 = string.byte(size_data, 1)
local b2 = string.byte(size_data, 2)
local b3 = string.byte(size_data, 3)
local b4 = string.byte(size_data, 4)

local image_size = bit.bor(bit.lshift(b4, 24), bit.lshift(b3, 16), bit.lshift(b2, 8), b1)

local image_data, err = sock:receive(image_size)
sock:setkeepalive()

if not image_data then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

ngx.header["Content-Length"] = image_size
ngx.header["Content-Type"] = "image/png"
ngx.say(image_data)
