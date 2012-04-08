#ifndef DDEBUG
#define DDEBUG 0
#endif

#include "ddebug.h"
#include "redis4nginx_module.h"

static void* redis4nginx_create_srv_conf(ngx_conf_t *cf);
static char* redis4nginx_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static void* redis4nginx_create_loc_conf(ngx_conf_t *cf);
static char *redis4nginx_exec_handler_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *redis4nginx_exec_return_handler_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
ngx_int_t redis4nginx_exec_handler(ngx_http_request_t *r);

static ngx_command_t  redis4nginx_commands[] = {
    {	ngx_string("redis_host"),
        NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_SRV_CONF_OFFSET,
        offsetof(redis4nginx_srv_conf_t, host),
        NULL },

    {	ngx_string("redis_port"),
        NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_SRV_CONF_OFFSET,
        offsetof(redis4nginx_srv_conf_t, port),
        NULL },
        
    {	ngx_string("redis_startup_script"),
        NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_SRV_CONF_OFFSET,
        offsetof(redis4nginx_srv_conf_t, startup_script),
        NULL },

    {   ngx_string("redis_exec"),
        NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_ANY,
        redis4nginx_exec_handler_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },
        
    {   ngx_string("redis_exec_return"),
        NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_ANY,
        redis4nginx_exec_return_handler_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },    
    ngx_null_command
};

static ngx_http_module_t  redis4nginx_module_ctx = {
  NULL,                          /* preconfiguration */
  NULL,                          /* postconfiguration */
  NULL,                          /* create main configuration */
  NULL,                          /* init main configuration */
  redis4nginx_create_srv_conf,  /* create server configuration */
  redis4nginx_merge_srv_conf,   /* merge server configuration */
  redis4nginx_create_loc_conf,  /* create location configuration */
  NULL                          /* merge location configuration char* redis4nginx_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);*/
};


ngx_module_t redis4nginx_module = {
  NGX_MODULE_V1,
  &redis4nginx_module_ctx,      /* module context */
  redis4nginx_commands,         /* module directives */
  NGX_HTTP_MODULE,              /* module type */
  NULL,                         /* init master */
  NULL,                         /* init module ngx_int_t redis4nginx_init_module(ngx_cycle_t *cycle);*/
  NULL,                         /* init process */
  NULL,                         /* init thread */
  NULL,                         /* exit thread */
  NULL,                         /* exit process */
  NULL,                         /* exit master */
  NGX_MODULE_V1_PADDING
};

static void* redis4nginx_create_srv_conf(ngx_conf_t *cf)
{
    redis4nginx_srv_conf_t *srv_conf;
    
    srv_conf = ngx_pcalloc(cf->pool, sizeof(redis4nginx_srv_conf_t));

    srv_conf->port = NGX_CONF_UNSET;

    return srv_conf;
}

static char* redis4nginx_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
	redis4nginx_srv_conf_t *prev = parent;
	redis4nginx_srv_conf_t *conf = child;

	ngx_log_debug0(NGX_LOG_INFO, cf->log, 0, "resis4nginx merge srv");

	ngx_conf_merge_str_value(conf->host,
			prev->host, NULL);

	ngx_conf_merge_value(conf->port,
			prev->port, NGX_CONF_UNSET);
        
	return NGX_CONF_OK;
}

static void* redis4nginx_create_loc_conf(ngx_conf_t *cf)
{
	redis4nginx_loc_conf_t *conf = ngx_pcalloc(cf->pool, sizeof(redis4nginx_loc_conf_t));
    
    ngx_array_init(&conf->directives, cf->pool, 1, sizeof(redis4nginx_directive_t));
    
	return conf;
}

static char *redis4nginx_exec_handler_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    redis4nginx_loc_conf_t *loc_conf = conf;
    redis4nginx_srv_conf_t *srv_conf;
    
    redis4nginx_directive_t *directive = ngx_array_push(&loc_conf->directives);
    directive->finalize = 0;
    directive->hash_elements = NULL;
    
    srv_conf = ngx_http_conf_get_module_srv_conf(cf, redis4nginx_module);
    
    return redis4nginx_compile_directive(cf, loc_conf, srv_conf, directive);
}

static char *redis4nginx_exec_return_handler_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    redis4nginx_loc_conf_t *loc_conf = conf;
    ngx_http_core_loc_conf_t *core_conf;
    redis4nginx_directive_t *directive;
    redis4nginx_srv_conf_t *srv_conf;
    
    core_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    if(core_conf->handler == NULL) {
        core_conf->handler = &redis4nginx_exec_handler;
    }
    
    directive = ngx_array_push(&loc_conf->directives);
    directive->finalize = 1;
    directive->hash_elements = NULL;
            
    srv_conf = ngx_http_conf_get_module_srv_conf(cf, redis4nginx_module);
    
    return redis4nginx_compile_directive(cf, loc_conf, srv_conf, directive);
}
