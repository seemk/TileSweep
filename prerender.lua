ngx.req.read_body()
local content = ngx.req.get_body_data()

local payload = cjson.decode(content)

if not payload then
  ngx.exit(ngx.HTTP_BAD_REQUEST)
end

local prerender_req = {
  type = 2,
  content = payload
}

local sock = ngx.socket.tcp()
sock:settimeout(60000)
local ok, err = sock:connect(tile_sv_host, tile_sv_port)
if not ok then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

ok, err = sock:send(cjson.encode(prerender_req))
local reader = sock:receiveuntil('\0')
local response = reader()
ngx.say(response)

sock:setkeepalive()

ngx.exit(ngx.HTTP_OK)
