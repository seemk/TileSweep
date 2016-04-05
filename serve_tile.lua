local sock = ngx.socket.udp();
local ok, err = sock:setpeername("127.0.0.1", 9567)
if not ok then
  ngx.say("conn fail: ", err)
  return
end

local w = ngx.var.w or 256
local h = ngx.var.h or 256

query = string.format("%d,%d,%d,%d,%d", ngx.var.z, ngx.var.x, ngx.var.y, w, h)
ngx.say(query)
ok, err = sock:send(query)

local data, err = sock:receive()
ngx.say(data)
sock:close()
