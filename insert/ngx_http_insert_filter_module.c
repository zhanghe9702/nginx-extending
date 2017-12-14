#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_insert_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static void * ngx_http_insert_filter_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_insert_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_http_insert_content(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_insert_filter_init(ngx_conf_t *cf);

static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_conf_enum_t  ngx_insert_filter[] = {
    { ngx_string("off"), 0 },
    { ngx_string("on"), 1 },
    { ngx_null_string, 0 }
};

typedef struct {
    ngx_http_complex_value_t body;
    ngx_uint_t enable;
} ngx_http_insert_filter_loc_conf_t ;    



static char *
ngx_http_insert_content(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_insert_filter_loc_conf_t *lcf = conf;

    ngx_str_t                         *value;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;


    value = cf->args->elts;

    if (cf->args->nelts == 2) {

		ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

		ccv.cf = cf;
		ccv.value = &value[1];
		ccv.complex_value = &cv;

		if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
			return NGX_CONF_ERROR;
		}



        lcf->body = cv;

        return NGX_CONF_OK;

    }
    return NGX_CONF_ERROR;
}

static ngx_command_t  ngx_http_insert_filter_commands[] = {

    { ngx_string("insert_content"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_insert_content,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_insert_filter_loc_conf_t, body),
      NULL },

    { ngx_string("insert_enable"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_insert_filter_loc_conf_t, enable),
      &ngx_insert_filter },

      ngx_null_command
};
static ngx_http_module_t  ngx_http_insert_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_insert_filter_init,           /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_insert_filter_create_loc_conf, /* create location configuration */
    ngx_http_insert_filter_merge_loc_conf, /* merge location configuration */
};

ngx_module_t  ngx_http_insert_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_insert_filter_module_ctx,    /* module context */
    ngx_http_insert_filter_commands,       /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_insert_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_insert_filter_loc_conf_t *conf = NULL;
    ngx_chain_t **ll;
    ngx_chain_t *cl;
    ngx_chain_t *nl;
    ngx_chain_t *out;
    ngx_buf_t *b;
	ngx_str_t value;

    if (r->header_only) {
        return ngx_http_next_body_filter(r, in);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_insert_filter_module);

	if (!conf->enable) {
		return ngx_http_next_body_filter(r, in);
	} 

	if (ngx_http_complex_value(r, &conf->body, &value) != NGX_OK) {
		return NGX_ERROR;
	}

    ll = &out; 
    for(cl = in; cl; cl = cl->next) {

        nl = ngx_alloc_chain_link(r->pool);
        if (!nl) {
            return NGX_ERROR;
        }
        b = ngx_create_temp_buf(r->pool, value.len + 1);
        /*b = ngx_calloc_buf(r->pool);*/
        b->memory = 1;
        b->last = ngx_cpymem(b->pos, value.data, value.len);

        nl->buf = b;
        *ll = nl;
        ll = &nl->next;

        nl = ngx_alloc_chain_link(r->pool);

        if (!nl) {
            return NGX_ERROR;
        }
        nl->buf = cl->buf;
        *ll = nl;
        ll = &nl->next;

    }
    *ll = NULL;
    return ngx_http_next_body_filter(r, out);
}

static ngx_int_t
ngx_http_insert_filter_init(ngx_conf_t *cf)
{

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_insert_body_filter;

    return NGX_OK;
}

static void *
ngx_http_insert_filter_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_insert_filter_loc_conf_t  *conf;

    conf = ngx_palloc(cf->pool, sizeof(ngx_http_insert_filter_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET_UINT;

    return conf;
}

static char *
ngx_http_insert_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_insert_filter_loc_conf_t *prev = parent;
    ngx_http_insert_filter_loc_conf_t *conf = child;

    if (conf->enable == NGX_CONF_UNSET_UINT) {
        conf->enable  = prev->enable;
    }

    return NGX_CONF_OK;
}
