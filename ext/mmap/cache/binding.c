
static VALUE eClosedError = Qnil;
static VALUE eMmapCacheModule = Qnil;

/******************************************************************************/

static int raise_if_closed(VALUE self)
{
  if (rb_ivar_get(self, rb_intern("@closed")) != Qtrue) return 0;
  rb_raise(eClosedError, "Map was freed");
  return 1;
}

static void mark_as_closed(VALUE self)
{
  rb_ivar_set(self, rb_intern("@closed"), Qtrue);
}

/******************************************************************************/

static void mmap_cache_free(void* haystack)
{
  int res = -1;

  if (haystack == NULL) return;
  // res = mmap_cache_storage_close((trigram_map*) &haystack);
  assert(res >= 0);
}

/******************************************************************************/

static void mmap_cache_mark(void* haystack)
{
  if (haystack == NULL) return;
  // mmap_cache_storage_mark((trigram_map) haystack);
}

/******************************************************************************/

static VALUE mmap_cache_new(VALUE class) {
  VALUE       wrapper  = Qnil;
  trigram_map haystack = (trigram_map)NULL;
  int         res      = -1;

  // res = mmap_cache_storage_new(&haystack);
  if (res < 0) { rb_sys_fail(NULL); return Qnil; }

  wrapper = Data_Wrap_Struct(class, mmap_cache_mark, mmap_cache_free, (void*)haystack);
  rb_obj_call_init(wrapper, 0, NULL);
  return wrapper;
}

/******************************************************************************/

static VALUE mmap_cache_load(VALUE class, VALUE rb_path) {
  char*       path     = StringValuePtr(rb_path);
  VALUE       wrapper  = Qnil;
  trigram_map haystack = (trigram_map)NULL;
  int         res      = -1;

  // res = mmap_cache_storage_load(&haystack, path);
  if (res < 0) { rb_sys_fail(NULL); return Qnil; }

  wrapper = Data_Wrap_Struct(class, mmap_cache_mark, mmap_cache_free, (void*)haystack);
  rb_obj_call_init(wrapper, 0, NULL);
  return wrapper;
}

/******************************************************************************/

static VALUE mmap_cache_initialize(VALUE UNUSED(self)) {
  return Qtrue;
}

/******************************************************************************/

static VALUE mmap_cache_put(VALUE self, VALUE rb_needle, VALUE rb_reference, VALUE rb_weight) {
  trigram_map  haystack  = (trigram_map)NULL;
  int          res       = -1;
  char*        needle    = StringValuePtr(rb_needle);
  uint32_t     reference = NUM2UINT(rb_reference);
  uint32_t     weight    = NUM2UINT(rb_weight);

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  // res = mmap_cache_storage_put(haystack, needle, reference, weight);
  assert(res >= 0);

  return INT2NUM(res);
}

/******************************************************************************/

static VALUE mmap_cache_delete(VALUE self, VALUE rb_reference) {
  trigram_map  haystack  = (trigram_map)NULL;
  uint32_t     reference = NUM2UINT(rb_reference);
  int          res       = -1;

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  // res = mmap_cache_storage_delete(haystack, reference);
  assert(res >= 0);

  return INT2NUM(res);
}

/******************************************************************************/

static VALUE mmap_cache_save(VALUE self, VALUE rb_path) {
  trigram_map  haystack  = (trigram_map)NULL;
  int          res       = -1;
  const char*  path      = StringValuePtr(rb_path);

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  // res = mmap_cache_storage_save(haystack, path);
  if (res < 0) rb_sys_fail(NULL);

  return Qnil;
}

/******************************************************************************/

static VALUE mmap_cache_find(VALUE self, VALUE rb_needle, VALUE rb_limit) {
  trigram_map   haystack   = (trigram_map)NULL;
  int           res        = -1;
  const char*   needle     = StringValuePtr(rb_needle);
  int           limit      = NUM2UINT(rb_limit);
  trigram_match matches    = NULL;
  VALUE         rb_matches = Qnil;

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  if (limit <= 0) {
    // rb_limit = rb_const_get(eMmapCacheModule, rb_intern('LIMIT_DEFAULT'));
    rb_limit = rb_const_get(eMmapCacheModule, rb_intern("LIMIT_DEFAULT"));
    limit = NUM2UINT(rb_limit);
  }
  matches = (trigram_match) malloc(limit * sizeof(trigram_match_t));

  // res = mmap_cache_storage_find(haystack, needle, limit, matches);
  assert(res >= 0);

  /* wrap the matches into a Ruby array */
  rb_matches = rb_ary_new();
  for (int k = 0; k < res; ++k) {
    VALUE rb_match = rb_ary_new();
    rb_ary_push(rb_match, rb_uint_new(matches[k].reference));
    rb_ary_push(rb_match, rb_uint_new(matches[k].matches));
    rb_ary_push(rb_match, rb_uint_new(matches[k].weight));
    rb_ary_push(rb_matches, rb_match);
  }
  return rb_matches;
}


/******************************************************************************/

static VALUE mmap_cache_stats(VALUE self)
{
  trigram_map     haystack = (trigram_map)NULL;
  trigram_stat_t  stats;
  VALUE           result   = rb_hash_new();
  int             res      = -1;

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  // res = mmap_cache_storage_stats(haystack, &stats);
  assert(res >= 0);

  (void) rb_hash_aset(result, ID2SYM(rb_intern("references")), UINT2NUM(stats.references));
  (void) rb_hash_aset(result, ID2SYM(rb_intern("trigrams")),   UINT2NUM(stats.trigrams));

  return result;
}

/******************************************************************************/

static VALUE mmap_cache_close(VALUE self)
{
  trigram_map     haystack = (trigram_map)NULL;
  int             res      = -1;

  if (raise_if_closed(self)) return Qnil;
  Data_Get_Struct(self, struct trigram_map_t, haystack);

  // res = mmap_cache_storage_close(&haystack);
  if (res < 0) rb_sys_fail(NULL);

  DATA_PTR(self) = NULL;
  mark_as_closed(self);
  return Qnil;
}

/******************************************************************************/

void Init_map_ext(void) {
  VALUE klass  = Qnil;

  /* assume we haven't yet defined mmap_cache */
  eMmapCacheModule = rb_define_module("MmapCache");
  assert(eMmapCacheModule != Qnil);

  klass = rb_define_class_under(eMmapCacheModule, "RawMap", rb_cObject);
  assert(klass != Qnil);

  eClosedError = rb_define_class_under(klass, "ClosedError", rb_eRuntimeError);
  assert(klass != Qnil);

  rb_define_singleton_method(klass, "new",  mmap_cache_new,  0);
  rb_define_singleton_method(klass, "load", mmap_cache_load, 1);

  rb_define_method(klass, "initialize", mmap_cache_initialize, 0);
  rb_define_method(klass, "put",        mmap_cache_put,        3);
  rb_define_method(klass, "delete",     mmap_cache_delete,     1);
  rb_define_method(klass, "save",       mmap_cache_save,       1);
  rb_define_method(klass, "find",       mmap_cache_find,       2);
  rb_define_method(klass, "stats",      mmap_cache_stats,      0);
  rb_define_method(klass, "close",      mmap_cache_close,      0);
  return;
}
