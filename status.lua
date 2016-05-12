local status_req = {
  type = 4,
  content = {}
}

local sock = ngx.socket.tcp()
sock:settimeout(60000)
local ok, err = sock:connect(tile_sv_host, tile_sv_port)
if not ok then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

ok, err = sock:send(cjson.encode(status_req))
local reader = sock:receiveuntil('\0')
local response = reader()
sock:setkeepalive()

ngx.say(response)
