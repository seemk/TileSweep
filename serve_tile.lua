local sock = ngx.socket.tcp();
sock:settimeout(1000);
local ok, err = sock:connect("127.0.0.1", 9567)
if not ok then
  ngx.say("conn fail: ", err)
  return
end

local w = ngx.var.w or 256
local h = ngx.var.h or 256

query = string.format("%d,%d,%d,%d,%d", ngx.var.z, ngx.var.x, ngx.var.y, w, h)
ngx.say("sending query");
ok, err = sock:send(query)

sock:settimeout(1000)
local size_data, err = sock:receive(4)

ngx.say(size_data)

sock:setkeepalive()

--ngx.header["Content-Length"] = string.len(buf)
--ngx.header["Content-Type"] = "image/png"
--ngx.say(buf)
