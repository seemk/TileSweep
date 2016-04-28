ngx.req.read_body()
local content = ngx.req.get_body_data()

-- Temporary
local payload = cjson.decode(content)

if not payload then
  ngx.exit(ngx.HTTP_BAD_REQUEST)
end

local prerender_req = {
  type = 2,
  content = cjson.encode(payload)
}

local sock = ngx.socket.tcp()
sock:settimeout(60000)
local ok, err = sock:connect(tile_sv_host, tile_sv_port)
if not ok then
  ngx.exit(ngx.HTTP_REQUEST_TIMEOUT)
end

ok, err = sock:send(query)
sock:setkeepalive()

if not ok then
  
end

ngx.exit(ngx.HTTP_OK)
