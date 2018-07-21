#include "tilesweep.h"
#include <fcntl.h>
#include <h2o.h>
#include <inttypes.h>
#include <signal.h>
#include "hash/xxhash.h"
#include "image.h"
#include "image_db.h"
#include "json/parson.h"
#include "platform.h"
#include "prerender.h"
#include "query.h"
#include "stretchy_buffer.h"
#include "taskpool.h"
#include "tcp.h"
#include "tile_renderer.h"
#include "ts_cache.h"
#include "ts_math.h"
#include "ts_options.h"
#include "ts_tile.h"
#include "ts_write_queue.h"

typedef enum { res_ok, res_failure } tilesweep_result;

typedef struct {
  h2o_context_t super;
  void* user;
} ts_h2o_ctx;

typedef struct {
  int32_t rendering_enabled;
  int32_t num_task_threads;
  int32_t num_http_threads;
  taskpool* task_pool;
  ts_write_queue* write_queue;
  tile_renderer** renderers;
  image_db* db_conns;
  tilesweep_stats* stats;
  atomic_ulong job_id_counter;
} tilesweep_shared_ctx;

typedef struct {
  ts_tile tile;
  image img;
  const tilesweep_shared_ctx* shared;
  int success;
} tile_render_task;

typedef struct {
  int32_t http_thread_idx;
  const ts_options* opt;
  tilesweep_shared_ctx* shared;
  int fd;
  pthread_t thread_id;
  ts_cache* cache;
  image_db* tile_db;
  h2o_accept_ctx_t accept_ctx;
  h2o_socket_t* socket;
  ts_h2o_ctx ctx;
} http_ctx;

static struct {
  h2o_globalconf_t globalconf;
  ts_options* opt;
} conf;

static void cache_item_expired(const ts_cache_item* item) { free(item->user); }

static void on_accept(h2o_socket_t* listener, const char* err) {
  if (err != NULL) {
    ts_log("accept error: %s", err);
    return;
  }

  h2o_socket_t* socket = h2o_evloop_socket_accept(listener);
  if (socket == NULL) {
    return;
  }

  http_ctx* c = (http_ctx*)listener->data;
  h2o_accept(&c->accept_ctx, socket);
}

int32_t http_ctx_init(http_ctx* c) {
  const ts_options* opt = c->opt;

  c->cache = ts_cache_create(
      &(ts_cache_options){.max_mem = opt->cache_size_bytes,
                          .lfu_log_factor = opt->cache_log_factor,
                          .lfu_decay_time = opt->cache_decay_seconds,
                          .on_item_expire = cache_item_expired});
  c->tile_db = image_db_open(opt->database);

  if (!c->tile_db) {
    return 0;
  }

  h2o_context_init(&c->ctx.super, h2o_evloop_create(), &conf.globalconf);
  c->ctx.user = c;
  c->accept_ctx.ctx = &c->ctx.super;
  c->accept_ctx.hosts = conf.globalconf.hosts;

  c->socket = h2o_evloop_socket_create(c->ctx.super.loop, c->fd,
                                       H2O_SOCKET_FLAG_DONT_READ);
  c->socket->data = c;
  h2o_socket_read_start(c->socket, on_accept);

  return 1;
}

static h2o_pathconf_t* register_handler(h2o_hostconf_t* conf, const char* path,
                                        int (*on_req)(h2o_handler_t*,
                                                      h2o_req_t*)) {
  h2o_pathconf_t* path_conf = h2o_config_register_path(conf, path, 0);
  h2o_handler_t* handler = h2o_create_handler(path_conf, sizeof(*handler));
  handler->on_req = on_req;

  return path_conf;
}

static void send_bad_request(h2o_req_t* req) {
  h2o_generator_t gen = {NULL, NULL};
  h2o_start_response(req, &gen);
  req->res.status = 400;
  req->res.reason = "Bad Request";
  h2o_send(req, &req->entity, 1, 1);
}

static void send_ok_request(h2o_req_t* req) {
  h2o_generator_t gen = {NULL, NULL};
  h2o_start_response(req, &gen);
  req->res.status = 200;
  req->res.reason = "Ok";
  h2o_send(req, &req->entity, 1, 1);
}

static void send_server_error(h2o_req_t* req) {
  h2o_generator_t gen = {NULL, NULL};
  h2o_start_response(req, &gen);
  req->res.status = 500;
  req->res.reason = "Internal Server Error";
  h2o_send(req, &req->entity, 1, 1);
}

static void send_image_response(h2o_req_t* req, const image* img) {
  req->res.status = 200;
  req->res.reason = "OK";

  h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE,
                 H2O_STRLIT("image/png"));

  char content_length[8];
  snprintf(content_length, 8, "%d", img->len);
  size_t header_length = strlen(content_length);
  h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_LENGTH,
                 content_length, header_length);

  h2o_iovec_t body;
  body.base = (char*)img->data;
  body.len = img->len;
  h2o_send(req, &body, 1, 1);
}

typedef struct {
  tilesweep_shared_ctx* shared;
  prerender_job_stats* stats;
  ts_tile t;
  image img;
} tile_render_args;

static void* render_tile_background(void* arg,
                                    const ts_task_extra_info* extra) {
  tile_render_args* args = (tile_render_args*)arg;
  tilesweep_shared_ctx* shared = args->shared;
  prerender_job_stats* stats = args->stats;

  const int32_t thread_idx = extra->executing_thread_idx;

  image_db* db = &shared->db_conns[thread_idx];
  if (image_db_exists(db, tile_hash(&args->t))) {
    ts_log("background render; tile [%d, %d, %d] [%d, %d] exists", args->t.x,
           args->t.y, args->t.z, args->t.w, args->t.h);
    goto bg_render_finish;
  }

  tile_renderer* renderer = shared->renderers[thread_idx];

  int32_t success = render_tile(renderer, &args->t, &args->img);

  if (success) {
    const uint64_t image_hash = XXH64(args->img.data, args->img.len, 0);
    ts_write_queue_push(shared->write_queue, args->t, args->img, image_hash);
    ts_log("background render; tile [%d, %d, %d] [%d, %d] done", args->t.x,
           args->t.y, args->t.z, args->t.w, args->t.h);
  } else {
    // TODO: Count failed tiles
    ts_log("background render; tile [%d, %d, %d] [%d, %d] fail", args->t.x,
           args->t.y, args->t.z, args->t.w, args->t.h);
    if (args->img.data) {
      free(args->img.data);
    }
  }

bg_render_finish:
  atomic_fetch_add(&stats->current_tiles, 1);

  if (atomic_load(&stats->indice_calcs_remain) == 0 &&
      atomic_load(&stats->current_tiles) ==
          atomic_load(&stats->num_tilecoords)) {
    tilesweep_stats_remove_prerender(shared->stats, stats);
    prerender_job_stats_destroy(stats);
    ts_write_queue_commit(shared->write_queue);
  }

  free(args);
  return NULL;
}

static void* calculate_tiles(void* arg, const ts_task_extra_info* extra) {
  tile_calc_job* job = (tile_calc_job*)arg;
  tilesweep_shared_ctx* shared = (tilesweep_shared_ctx*)job->user;

  int32_t num_tiles = 0;
  vec2i* tiles = calc_tiles(job, &num_tiles);

  prerender_job_stats* stats = job->stats;
  atomic_fetch_add(&stats->num_tilecoords, num_tiles);

  for (int32_t i = 0; i < num_tiles; i++) {
    vec2i t = tiles[i];
    tile_render_args* render_args =
        (tile_render_args*)calloc(1, sizeof(tile_render_args));
    render_args->shared = shared;
    render_args->stats = stats;
    render_args->t = (ts_tile){.x = t.x,
                               .y = t.y,
                               .z = job->zoom,
                               .w = job->tile_size,
                               .h = job->tile_size};

    ts_task* tsk = ts_task_create(render_tile_background, render_args);
    taskpool_post(shared->task_pool, tsk, TP_MED);
  }

  if (job->fill_state.finished) {
    atomic_fetch_sub(&stats->indice_calcs_remain, 1);
    ts_log("tile calc (id=%d, zoom=%d) job finished", job->id, job->zoom);

    fill_poly_state_destroy(&job->fill_state);
    free(job);
  } else {
    taskpool_post(shared->task_pool, ts_task_create(calculate_tiles, job),
                  TP_LOW);
  }

  free(tiles);

  return NULL;
}

static void* setup_prerender(void* arg, const ts_task_extra_info* info) {
  prerender_req* req = (prerender_req*)arg;
  tilesweep_shared_ctx* shared = (tilesweep_shared_ctx*)req->user;
  ts_log("setting up prerender. min_zoom: %d, max_zoom %d, tile_size: %d",
         req->min_zoom, req->max_zoom, req->tile_size);

  tile_calc_job** jobs =
      make_tile_calc_jobs(req->coordinates, req->num_coordinates, req->min_zoom,
                          req->max_zoom, req->tile_size, 8096);

  const uint64_t job_id = atomic_fetch_add(&shared->job_id_counter, 1);
  const int32_t num_tilecoord_jobs = sb_count(jobs);

  uint64_t tile_estimate = 0;
  for (int32_t i = 0; i < num_tilecoord_jobs; i++) {
    tile_estimate += jobs[i]->estimated_tiles;
  }

  prerender_job_stats* stats = prerender_job_stats_create(
      req->coordinates, req->num_coordinates, job_id, num_tilecoord_jobs);
  stats->estimated_tiles = tile_estimate;
  stats->min_zoom = req->min_zoom;
  stats->max_zoom = req->max_zoom;

  tilesweep_stats_add_prerender(shared->stats, stats);

  for (int32_t j = 0; j < num_tilecoord_jobs; j++) {
    tile_calc_job* job = jobs[j];
    job->id = req->id;
    job->user = req->user;
    job->stats = stats;

    ts_task* tiles_task = ts_task_create(calculate_tiles, job);
    taskpool_post(shared->task_pool, tiles_task, TP_LOW);
  }

  sb_free(jobs);
  free(req->coordinates);
  free(req);

  return NULL;
}

static int start_prerender(h2o_handler_t* h, h2o_req_t* req) {
  if (req->entity.base == NULL) return -1;

  char* body = (char*)calloc(req->entity.len + 1, 1);
  memcpy(body, req->entity.base, req->entity.len);

  JSON_Value* root = json_parse_string(body);
  free(body);

  if (root == NULL || json_value_get_type(root) != JSONObject) {
    send_bad_request(req);
    if (root) json_value_free(root);
    return 0;
  }

  JSON_Object* req_json = json_value_get_object(root);

  JSON_Array* coords_json = json_object_get_array(req_json, "coordinates");
  int32_t min_zoom = (int32_t)json_object_get_number(req_json, "minZoom");
  int32_t max_zoom = (int32_t)json_object_get_number(req_json, "maxZoom");
  int32_t tile_size = (int32_t)json_object_get_number(req_json, "tileSize");

  if (tile_size != 256 && tile_size != 512) {
    tile_size = 256;
  }

  if (coords_json == NULL || min_zoom < 0.0 || max_zoom < 0.0 ||
      min_zoom > MAX_ZOOM_LEVEL || max_zoom > MAX_ZOOM_LEVEL) {
    send_bad_request(req);
    json_value_free(root);
    return 0;
  }

  size_t num_coords = json_array_get_count(coords_json);

  ts_h2o_ctx* h2o = (ts_h2o_ctx*)req->conn->ctx;
  http_ctx* c = (http_ctx*)h2o->user;
  tilesweep_shared_ctx* shared = c->shared;

  prerender_req* prerender = (prerender_req*)calloc(1, sizeof(prerender_req));
  prerender->min_zoom = min_zoom;
  prerender->max_zoom = max_zoom;
  prerender->coordinates = (vec2d*)calloc(num_coords, sizeof(vec2d));
  prerender->num_coordinates = num_coords;
  prerender->tile_size = tile_size;
  prerender->user = shared;

  for (int i = 0; i < num_coords; i++) {
    JSON_Array* xy_json = json_array_get_array(coords_json, i);
    if (xy_json) {
      int coord_len = json_array_get_count(xy_json);
      if (coord_len == 2) {
        double x = json_array_get_number(xy_json, 0);
        double y = json_array_get_number(xy_json, 1);
        prerender->coordinates[i].x = x;
        prerender->coordinates[i].y = y;
      }
    }
  }

  json_value_free(root);

  ts_task* setup_prerender_task = ts_task_create(setup_prerender, prerender);
  taskpool_post(shared->task_pool, setup_prerender_task, TP_LOW);

  send_ok_request(req);
  return 0;
}

static void* render_tile_handler(void* arg, const ts_task_extra_info* extra) {
  tile_render_task* task = (tile_render_task*)arg;
  tile_renderer* renderer =
      task->shared->renderers[extra->executing_thread_idx];
  task->success = render_tile(renderer, &task->tile, &task->img);
  return NULL;
}

static int serve_tile(h2o_handler_t* h, h2o_req_t* req) {
  int64_t req_start = usec_now();

  ts_h2o_ctx* h2o = (ts_h2o_ctx*)req->conn->ctx;
  http_ctx* c = (http_ctx*)h2o->user;
  ts_cache* cache = c->cache;
  tilesweep_shared_ctx* shared = c->shared;
  h2o_generator_t gen = {NULL, NULL};
  ts_tile t = parse_tile(req->path.base, req->path.len);

  h2o_start_response(req, &gen);

  if (!tile_valid(&t) || (t.w != 256 && t.w != 512) ||
      (t.h != 256 && t.h != 512)) {
    req->res.status = 400;
    req->res.reason = "Bad Request";
    h2o_send(req, &req->entity, 1, 1);
    return 0;
  }

  query_param_t params[4];
  int32_t n_params = parse_uri_params(req->path.base, req->path.len, params, 4);

  int32_t force_cache = 0;
  for (int32_t i = 0; i < n_params; i++) {
    const query_param_t* p = &params[i];
    if (h2o_memis(p->query, p->query_len, H2O_STRLIT("cached")) &&
        h2o_memis(p->value, p->value_len, H2O_STRLIT("true"))) {
      force_cache = 1;
    }
  }

  const uint64_t pos_hash = tile_hash(&t);
  image img = {0};

  // Use the upper 16 bits to store log2(dimension), assumes w, h are pow2
  uint64_t base = (uint64_t)log2pow2(t.w);
  uint64_t mem_cache_key = (base << 48) | pos_hash;

  ts_cache_item cache_item;
  if (ts_cache_get(cache, mem_cache_key, &cache_item)) {
    img.width = t.w;
    img.height = t.h;
    img.len = cache_item.size;
    img.data = (uint8_t*)cache_item.user;

    send_image_response(req, &img);

    ts_log("[%d, %d, %d, %d, %d] (%d bytes) | %.3f ms [cache: memory]", t.w,
           t.h, t.z, t.x, t.y, img.len, (double)(usec_now() - req_start) / 1e3);

    return 0;
  }

  if (image_db_fetch(c->tile_db, pos_hash, t.w, t.h, &img)) {
    send_image_response(req, &img);

    cache_item.key = mem_cache_key;
    cache_item.size = img.len;
    cache_item.user = img.data;

    if (!ts_cache_set(cache, &cache_item)) {
      free(img.data);
    }

    ts_log("[%d, %d, %d, %d, %d] (%d bytes) | %.3f ms [cache: disk]", t.w, t.h,
           t.z, t.x, t.y, img.len, (double)(usec_now() - req_start) / 1e3);

    return 0;
  }

  tilesweep_result result = res_ok;

  if (!force_cache && shared->rendering_enabled) {
    tile_render_task renderer_info = {
        .tile = t, .img = img, .shared = shared, .success = 0};

    ts_task* render_task = ts_task_create(render_tile_handler, &renderer_info);
    taskpool_wait(shared->task_pool, render_task, TP_HIGH);
    ts_task_destroy(render_task);

    if (renderer_info.success) {
      img = renderer_info.img;
      uint64_t image_hash = XXH64(img.data, img.len, 0);

      image write_img = {.width = img.width,
                         .height = img.height,
                         .len = img.len,
                         .data = (uint8_t*)calloc(img.len, 1)};

      memcpy(write_img.data, img.data, img.len);
      ts_write_queue_push(shared->write_queue, t, write_img, image_hash);
    } else {
      result = res_failure;
    }
  }

  if (img.len > 0) {
    send_image_response(req, &img);
    free(img.data);

    ts_log("[%d, %d, %d, %d, %d] (%d bytes) | %.2f ms [rendered]", t.w, t.h,
           t.z, t.x, t.y, img.len, (usec_now() - req_start) / 1000.0);

  } else {
    if (result == res_failure) {
      req->res.status = 500;
      req->res.reason = "Internal Server Error";
    } else {
      req->res.status = 204;
      req->res.reason = "No Content";
    }
    h2o_send(req, &req->entity, 1, 1);
  }

  return 0;
}

void* run_loop(void* arg) {
  http_ctx* c = (http_ctx*)arg;

  if (!http_ctx_init(c)) {
    ts_log("http_ctx_init failed");
    return NULL;
  }

  ts_log("http [%d] ready [cache %.3f MB]", c->http_thread_idx,
         (double)c->opt->cache_size_bytes / 1e6);

  while (h2o_evloop_run(c->ctx.super.loop, 100) == 0) {
    uint32_t time = usec_now() / UINT64_C(1000000);
    ts_cache_update(c->cache, time);
  }

  return NULL;
}

void set_signal_handler(int sig_num, void (*handler)(int sig_num)) {
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  sigaction(sig_num, &action, NULL);
}

typedef struct {
  ts_options* options;
  tilesweep_shared_ctx* shared;
  int32_t renderer_idx;
} renderer_create_args;

static void* setup_renderer(void* arg, const ts_task_extra_info* extra) {
  (void)extra;
  renderer_create_args* args = (renderer_create_args*)arg;
  ts_options* opt = args->options;
  tile_renderer* renderer =
      tile_renderer_create(opt->mapnik_xml, opt->plugins, opt->fonts);
  args->shared->renderers[args->renderer_idx] = renderer;
  return NULL;
}

static int get_status(h2o_handler_t* h, h2o_req_t* req) {
  if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET"))) {
    return -1;
  }

  ts_h2o_ctx* h2o = (ts_h2o_ctx*)req->conn->ctx;
  http_ctx* c = (http_ctx*)h2o->user;
  tilesweep_shared_ctx* shared = c->shared;
  tilesweep_stats* stats = shared->stats;

  JSON_Value* root = json_value_init_object();
  JSON_Object* root_obj = json_value_get_object(root);

  json_object_set_value(root_obj, "renders", json_value_init_array());
  JSON_Array* renders = json_object_get_array(root_obj, "renders");

  pthread_mutex_lock(&stats->lock);
  for (int32_t i = 0; i < sb_count(stats->prerenders); i++) {
    prerender_job_stats* job_stats = stats->prerenders[i];
    JSON_Value* prerender_val = json_value_init_object();
    JSON_Object* prerender_json = json_object(prerender_val);

    const double max_tiles = (double)atomic_load_explicit(&job_stats->max_tiles,
                                                          memory_order_relaxed);
    const double current_tiles = (double)atomic_load_explicit(
        &job_stats->current_tiles, memory_order_relaxed);

    json_object_set_number(prerender_json, "id", job_stats->id);
    json_object_set_number(prerender_json, "maxTiles", max_tiles);
    json_object_set_number(prerender_json, "numCurrentTiles", current_tiles);
    json_object_set_number(prerender_json, "numEstimatedTiles",
                           job_stats->estimated_tiles);
    json_object_set_number(prerender_json, "minZoom", job_stats->min_zoom);
    json_object_set_number(prerender_json, "maxZoom", job_stats->max_zoom);

    json_object_set_value(prerender_json, "boundary", json_value_init_array());
    JSON_Array* boundary_json =
        json_object_get_array(prerender_json, "boundary");

    for (int32_t c = 0; c < job_stats->num_coordinates; c++) {
      vec2d coordinate = job_stats->coordinates[c];
      JSON_Value* coordinate_val = json_value_init_array();
      JSON_Array* coord_arr = json_array(coordinate_val);

      json_array_append_number(coord_arr, coordinate.x);
      json_array_append_number(coord_arr, coordinate.y);
      json_array_append_value(boundary_json, coordinate_val);
    }

    json_array_append_value(renders, prerender_val);
  }
  pthread_mutex_unlock(&stats->lock);

  char* json = json_serialize_to_string(root);
  json_value_free(root);

  h2o_generator_t gen = {NULL, NULL};
  h2o_start_response(req, &gen);
  req->res.status = 200;
  req->res.reason = "Ok";

  h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE,
                 H2O_STRLIT("application/json"));

  h2o_iovec_t body;
  body.base = json;
  body.len = strlen(json);
  h2o_send(req, &body, 1, 1);

  json_free_serialized_string(json);

  return 0;
}

int main(int argc, char** argv) {
  set_signal_handler(SIGPIPE, SIG_IGN);

  ts_options opt = ts_options_parse(argc, argv);
  conf.opt = &opt;

  tilesweep_shared_ctx* shared =
      (tilesweep_shared_ctx*)calloc(1, sizeof(tilesweep_shared_ctx));
#if TC_NO_MAPNIK
  shared->rendering_enabled = 0;
#else
  shared->rendering_enabled = strcmp(opt.rendering, "1") == 0;
#endif
  shared->num_task_threads = cpu_core_count();
  shared->num_http_threads = shared->num_task_threads;
  shared->task_pool = taskpool_create(shared->num_task_threads);
  shared->write_queue = ts_write_queue_create(image_db_open(opt.database));
  shared->renderers =
      (tile_renderer**)calloc(shared->num_task_threads, sizeof(tile_renderer*));

  shared->stats = tilesweep_stats_create();
  atomic_init(&shared->job_id_counter, 1);

  ts_task** startup_tasks =
      (ts_task**)calloc(shared->num_task_threads, sizeof(ts_task*));

  renderer_create_args* rc_args = (renderer_create_args*)calloc(
      shared->num_task_threads, sizeof(renderer_create_args));

  if (shared->rendering_enabled) {
    shared->db_conns =
        (image_db*)calloc(shared->num_task_threads, sizeof(image_db));
    for (int32_t i = 0; i < shared->num_task_threads; i++) {
      rc_args[i] = (renderer_create_args){
          .options = &opt, .shared = shared, .renderer_idx = i};
      if (!image_db_init(&shared->db_conns[i], opt.database)) {
        ts_log("failed to open image db %d %s", i, opt.database);
        return 1;
      }

      ts_task* t = ts_task_create(setup_renderer, &rc_args[i]);
      startup_tasks[i] = t;
    }

    taskpool_wait_all(shared->task_pool, startup_tasks,
                      shared->num_task_threads, TP_HIGH);

    for (int32_t i = 0; i < shared->num_task_threads; i++) {
      ts_task_destroy(startup_tasks[i]);
    }
  }

  h2o_config_init(&conf.globalconf);

  h2o_hostconf_t* hostconf = h2o_config_register_host(
      &conf.globalconf, h2o_iovec_init(H2O_STRLIT("default")), 65535);
  register_handler(hostconf, "/tile", serve_tile);

  if (shared->rendering_enabled) {
    register_handler(hostconf, "/prerender", start_prerender);
  }

  register_handler(hostconf, "/status", get_status);
  h2o_file_register(h2o_config_register_path(hostconf, "/", 0), "ui", NULL,
                    NULL, 0);

  int sock_fd = bind_tcp(opt.host, opt.port);
  if (sock_fd == -1) {
    ts_log("failed to bind socket");
    return 1;
  }

  http_ctx* http_contexts =
      (http_ctx*)calloc(shared->num_http_threads, sizeof(http_ctx));

  for (int32_t i = 0; i < shared->num_http_threads; i++) {
    http_ctx* c = &http_contexts[i];
    c->http_thread_idx = i;
    c->opt = &opt;
    c->shared = shared;
    c->fd = sock_fd;
  }

  for (int32_t i = 1; i < shared->num_http_threads; i++) {
    http_ctx* c = &http_contexts[i];
    h2o_multithread_create_thread(&c->thread_id, NULL, &run_loop, c);
  }

  run_loop(&http_contexts[0]);

  taskpool_destroy(shared->task_pool);

  return 0;
}
