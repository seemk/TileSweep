local sock = ngx.socket.udp();
local ok, err = sock:setpeername("127.0.0.1", 9567)
if not ok then
  ngx.say("conn fail: ", err)
  return
end

local w = ngx.var.w or 256
local h = ngx.var.h or 256

query = string.format("%d,%d,%d,%d,%d", ngx.var.z, ngx.var.x, ngx.var.y, w, h)
ok, err = sock:send(query)

sock:settimeout(1000)

buf = ""
while true do
  local data, err = sock:receive()
  if not data then break end

  buf = buf .. data
end
sock:close()

ngx.header["Content-Length"] = string.len(buf)
ngx.header["Content-Type"] = "image/png"
ngx.say(buf)
