local sock = ngx.socket.tcp();
sock:settimeout(60000)
local ok, err = sock:connect(tile_sv_host, tile_sv_port)
if not ok then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

local w = ngx.var.w or 256
local h = ngx.var.h or 256

local query = {
  type = 1,
  content = {
    z = tonumber(ngx.var.z),
    x = tonumber(ngx.var.x),
    y = tonumber(ngx.var.y),
    w = tonumber(w),
    h = tonumber(h) 
  }
}
ok, err = sock:send(cjson.encode(query))

local reader = sock:receiveuntil("\0")

local res_json, err = reader()

if not res_json then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

local res = cjson.decode(res_json)
if not res then
  ngx.exit(ngx.ERROR)
end

local status = res["status"]
if status ~= 0 then
  ngx.exit(ngx.HTTP_NO_CONTENT)
end

local image_size = res["size"]

if not image_size then
  ngx.exit(ngx.ERROR)
end

local image_data, err = sock:receive(image_size)
sock:setkeepalive()

if not image_data then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

ngx.header["Content-Length"] = image_size
ngx.header["Content-Type"] = "image/png"
ngx.say(image_data)
