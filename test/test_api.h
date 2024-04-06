#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "api"

#if UFBXT_IMPL

static void ufbxt_close_memory(void *user, void *data, size_t data_size)
{
	free(data);
}

static bool ufbxt_open_file_memory_default(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	bool ok = ufbx_open_memory(stream, data, size, NULL, NULL);
	free(data);
	return ok;
}

static bool ufbxt_open_file_memory_temp(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	ufbx_open_memory_opts opts = { 0 };
	opts.allocator.allocator = info->temp_allocator;

	bool ok = ufbx_open_memory(stream, data, size, &opts, NULL);
	free(data);
	return ok;
}

static bool ufbxt_open_file_memory_ref(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	ufbx_open_memory_opts opts = { 0 };
	opts.no_copy = true;
	opts.close_cb.fn = &ufbxt_close_memory;
	return ufbx_open_memory(stream, data, size, &opts, NULL);
}

#endif

#if UFBXT_IMPL
static void ufbxt_do_open_memory_test(const char *filename, size_t expected_calls_fbx, size_t expected_calls_obj, ufbx_open_file_fn *open_file_fn)
{
	char path[512];
	ufbxt_file_iterator iter = { filename };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i < 2; i++) {
			ufbx_load_opts opts = { 0 };
			size_t num_calls = 0;

			opts.open_file_cb.fn = open_file_fn;
			opts.open_file_cb.user = &num_calls;
			opts.load_external_files = true;
			if (i == 1) {
				opts.read_buffer_size = 1;
			}

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);

			if (scene->metadata.file_format == UFBX_FILE_FORMAT_FBX) {
				ufbxt_assert(num_calls == expected_calls_fbx);
			} else if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
				ufbxt_assert(num_calls == expected_calls_obj);
			} else {
				ufbxt_assert(false);
			}

			ufbx_free_scene(scene);
		}
	}
}
#endif

UFBXT_TEST(open_memory_default)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_default);
}
#endif

UFBXT_TEST(open_memory_temp)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_temp);
}
#endif

UFBXT_TEST(open_memory_ref)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_ref);
}
#endif

UFBXT_TEST(obj_open_memory_default)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_default);
}
#endif

UFBXT_TEST(obj_open_memory_temp)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_temp);
}
#endif

UFBXT_TEST(obj_open_memory_ref)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_ref);
}
#endif

UFBXT_TEST(retain_free_null)
#if UFBXT_IMPL
{
	ufbx_retain_scene(NULL);
	ufbx_free_scene(NULL);
	ufbx_retain_mesh(NULL);
	ufbx_free_mesh(NULL);
	ufbx_retain_line_curve(NULL);
	ufbx_free_line_curve(NULL);
	ufbx_retain_geometry_cache(NULL);
	ufbx_free_geometry_cache(NULL);
	ufbx_retain_anim(NULL);
	ufbx_free_anim(NULL);
	ufbx_retain_baked_anim(NULL);
	ufbx_free_baked_anim(NULL);
}
#endif

UFBXT_TEST(thread_memory)
#if UFBXT_IMPL
{
	ufbx_retain_scene(NULL);
	ufbx_free_scene(NULL);
	ufbx_retain_mesh(NULL);
	ufbx_free_mesh(NULL);
	ufbx_retain_line_curve(NULL);
	ufbx_free_line_curve(NULL);
	ufbx_retain_geometry_cache(NULL);
	ufbx_free_geometry_cache(NULL);
	ufbx_retain_anim(NULL);
	ufbx_free_anim(NULL);
	ufbx_retain_baked_anim(NULL);
	ufbx_free_baked_anim(NULL);
}
#endif

#if UFBXT_IMPL
typedef struct {
	bool immediate;
	bool initialized;
	bool freed;
	uint32_t wait_index;
	uint32_t dispatches;
} ufbxt_single_thread_pool;

static bool ufbxt_single_thread_pool_init_fn(void *user, ufbx_thread_pool_context ctx, const ufbx_thread_pool_info *info)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	pool->initialized = true;

	return true;
}

static bool ufbxt_single_thread_pool_run_fn(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t start_index, uint32_t count)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	ufbxt_assert(pool->initialized);
	pool->dispatches++;
	if (!pool->immediate) return true;

	for (uint32_t i = 0; i < count; i++) {
		ufbx_thread_pool_run_task(ctx, start_index + i);
	}

	return true;
}

static bool ufbxt_single_thread_pool_wait_fn(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t max_index)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	ufbxt_assert(pool->initialized);

	if (!pool->immediate) {
		for (uint32_t i = pool->wait_index; i < max_index; i++) {
			ufbx_thread_pool_run_task(ctx, i);
		}
	}

	pool->wait_index = max_index;

	return true;
}

static void ufbxt_single_thread_pool_free_fn(void *user, ufbx_thread_pool_context ctx)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	pool->freed = true;
}

static void ufbxt_single_thread_pool_init(ufbx_thread_pool *dst, ufbxt_single_thread_pool *pool, bool immediate)
{
	memset(pool, 0, sizeof(ufbxt_single_thread_pool));
	pool->immediate = immediate;

	dst->init_fn = ufbxt_single_thread_pool_init_fn;
	dst->run_fn = ufbxt_single_thread_pool_run_fn;
	dst->wait_fn = ufbxt_single_thread_pool_wait_fn;
	dst->free_fn = ufbxt_single_thread_pool_free_fn;
	dst->user = pool;
}
#endif

#if UFBXT_IMPL
static bool ufbxt_is_big_endian()
{
		uint8_t buf[2];
		uint16_t val = 0xbbaa;
		memcpy(buf, &val, 2);
		return buf[0] == 0xbb;
}
#endif

UFBXT_TEST(single_thread_immediate_stream)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(single_thread_immediate_memory)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

		size_t size = 0;
		void *data = ufbxt_read_file(path, &size);
		ufbxt_assert(data);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
		free(data);
	}
}
#endif

UFBXT_TEST(single_thread_deferred_stream)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, false);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(single_thread_deferred_memory)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, false);

		size_t size = 0;
		void *data = ufbxt_read_file(path, &size);
		ufbxt_assert(data);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
		free(data);
	}
}
#endif

UFBXT_TEST(thread_memory_limit)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	size_t prev_dispatches = 0;
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i < 24; i++) {
			ufbxt_single_thread_pool pool;
			ufbx_load_opts opts = { 0 };
			ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);
			opts.thread_opts.memory_limit = (size_t)1 << i;

			size_t size = 0;
			void *data = ufbxt_read_file(path, &size);
			ufbxt_assert(data);

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			if (pool.dispatches != prev_dispatches) {
				ufbxt_logf("limit %zu dispatches: %u", opts.thread_opts.memory_limit, pool.dispatches);
				prev_dispatches = pool.dispatches;
			}

			ufbxt_assert(pool.initialized);
			ufbxt_assert(pool.freed);
			if (ufbxt_is_big_endian()) {
				ufbxt_assert(pool.wait_index == 0);
			} else {
				ufbxt_assert(pool.wait_index >= 100);
			}

			ufbxt_check_scene(scene);
			ufbx_free_scene(scene);
			free(data);
		}
	}
}
#endif

UFBXT_TEST(single_thread_file_not_found)
#if UFBXT_IMPL
{
	ufbxt_single_thread_pool pool;
	ufbx_load_opts opts = { 0 };
	ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file("<doesnotexist>.fbx", &opts, &error);
	ufbxt_assert(!scene);
	ufbxt_assert(error.type == UFBX_ERROR_FILE_NOT_FOUND);
	ufbxt_assert(strstr(error.info, "<doesnotexist>.fbx"));

	ufbxt_assert(!pool.initialized);
	ufbxt_assert(!pool.freed);
	ufbxt_assert(pool.wait_index == 0);
}
#endif

UFBXT_TEST(empty_file_memory)
#if UFBXT_IMPL
{
	{
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(NULL, 0, NULL, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}

	{
		ufbx_load_opts opts = { 0 };
		opts.file_format = UFBX_FILE_FORMAT_FBX;
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(NULL, 0, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}
}
#endif

#if UFBXT_IMPL
static size_t ufbxt_empty_stream_read_fn(void *user, void *data, size_t size)
{
	return 0;
}
#endif

UFBXT_TEST(empty_file_stream)
#if UFBXT_IMPL
{
	ufbx_stream stream = { 0 };
	stream.read_fn = &ufbxt_empty_stream_read_fn;

	{
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_stream(&stream, NULL, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}

	{
		ufbx_load_opts opts = { 0 };
		opts.file_format = UFBX_FILE_FORMAT_FBX;
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_stream(&stream, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}
}
#endif

UFBXT_TEST(file_format_lookahead)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i <= 16; i++) {
			ufbxt_hintf("i=%zu", i);

			ufbx_load_opts opts = { 0 };
			opts.file_format_lookahead = i * i * i;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbx_free_scene(scene);
		}
	}
}
#endif

#if UFBXT_IMPL
static void *ufbxt_multiuse_realloc(void *user, void *old_ptr, size_t old_size, size_t new_size)
{
	if (new_size == 0) {
		free(old_ptr);
		return NULL;
	} else if (old_size > 0) {
		return realloc(old_ptr, new_size);
	} else {
		return malloc(new_size);
	}
}
static void *ufbxt_null_realloc(void *user, void *old_ptr, size_t old_size, size_t new_size)
{
	return NULL;
}
static void *ufbxt_alloc_malloc(void *user, size_t size)
{
	return malloc(size);
}
static void ufbxt_free_free(void *user, void *ptr, size_t size)
{
	free(ptr);
}
#endif

UFBXT_TEST(multiuse_realloc)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.temp_allocator.allocator.realloc_fn = &ufbxt_multiuse_realloc;
		opts.result_allocator.allocator.realloc_fn = &ufbxt_multiuse_realloc;

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);
		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(null_realloc)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.temp_allocator.allocator.alloc_fn = &ufbxt_alloc_malloc;
		opts.temp_allocator.allocator.realloc_fn = &ufbxt_null_realloc;
		opts.temp_allocator.allocator.free_fn = &ufbxt_free_free;

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
	}
}
#endif

UFBXT_TEST(null_realloc_only)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		{
			ufbx_load_opts opts = { 0 };
			opts.temp_allocator.allocator.realloc_fn = &ufbxt_null_realloc;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			ufbxt_assert(!scene);
			ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
		}
		{
			ufbx_load_opts opts = { 0 };
			opts.result_allocator.allocator.realloc_fn = &ufbxt_null_realloc;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			ufbxt_assert(!scene);
			ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
		}
	}
}
#endif

#if UFBXT_IMPL
typedef struct {
	ufbx_element_type type;
	size_t size;
} ufbxt_element_type_size;
#endif

UFBXT_TEST(element_type_sizes)
#if UFBXT_IMPL
{
	const ufbxt_element_type_size element_type_sizes[] = {
		{ UFBX_ELEMENT_UNKNOWN, sizeof(ufbx_unknown) },
		{ UFBX_ELEMENT_NODE, sizeof(ufbx_node) },
		{ UFBX_ELEMENT_MESH, sizeof(ufbx_mesh) },
		{ UFBX_ELEMENT_LIGHT, sizeof(ufbx_light) },
		{ UFBX_ELEMENT_CAMERA, sizeof(ufbx_camera) },
		{ UFBX_ELEMENT_BONE, sizeof(ufbx_bone) },
		{ UFBX_ELEMENT_EMPTY, sizeof(ufbx_empty) },
		{ UFBX_ELEMENT_LINE_CURVE, sizeof(ufbx_line_curve) },
		{ UFBX_ELEMENT_NURBS_CURVE, sizeof(ufbx_nurbs_curve) },
		{ UFBX_ELEMENT_NURBS_SURFACE, sizeof(ufbx_nurbs_surface) },
		{ UFBX_ELEMENT_NURBS_TRIM_SURFACE, sizeof(ufbx_nurbs_trim_surface) },
		{ UFBX_ELEMENT_NURBS_TRIM_BOUNDARY, sizeof(ufbx_nurbs_trim_boundary) },
		{ UFBX_ELEMENT_PROCEDURAL_GEOMETRY, sizeof(ufbx_procedural_geometry) },
		{ UFBX_ELEMENT_STEREO_CAMERA, sizeof(ufbx_stereo_camera) },
		{ UFBX_ELEMENT_CAMERA_SWITCHER, sizeof(ufbx_camera_switcher) },
		{ UFBX_ELEMENT_MARKER, sizeof(ufbx_marker) },
		{ UFBX_ELEMENT_LOD_GROUP, sizeof(ufbx_lod_group) },
		{ UFBX_ELEMENT_SKIN_DEFORMER, sizeof(ufbx_skin_deformer) },
		{ UFBX_ELEMENT_SKIN_CLUSTER, sizeof(ufbx_skin_cluster) },
		{ UFBX_ELEMENT_BLEND_DEFORMER, sizeof(ufbx_blend_deformer) },
		{ UFBX_ELEMENT_BLEND_CHANNEL, sizeof(ufbx_blend_channel) },
		{ UFBX_ELEMENT_BLEND_SHAPE, sizeof(ufbx_blend_shape) },
		{ UFBX_ELEMENT_CACHE_DEFORMER, sizeof(ufbx_cache_deformer) },
		{ UFBX_ELEMENT_CACHE_FILE, sizeof(ufbx_cache_file) },
		{ UFBX_ELEMENT_MATERIAL, sizeof(ufbx_material) },
		{ UFBX_ELEMENT_TEXTURE, sizeof(ufbx_texture) },
		{ UFBX_ELEMENT_VIDEO, sizeof(ufbx_video) },
		{ UFBX_ELEMENT_SHADER, sizeof(ufbx_shader) },
		{ UFBX_ELEMENT_SHADER_BINDING, sizeof(ufbx_shader_binding) },
		{ UFBX_ELEMENT_ANIM_STACK, sizeof(ufbx_anim_stack) },
		{ UFBX_ELEMENT_ANIM_LAYER, sizeof(ufbx_anim_layer) },
		{ UFBX_ELEMENT_ANIM_VALUE, sizeof(ufbx_anim_value) },
		{ UFBX_ELEMENT_ANIM_CURVE, sizeof(ufbx_anim_curve) },
		{ UFBX_ELEMENT_DISPLAY_LAYER, sizeof(ufbx_display_layer) },
		{ UFBX_ELEMENT_SELECTION_SET, sizeof(ufbx_selection_set) },
		{ UFBX_ELEMENT_SELECTION_NODE, sizeof(ufbx_selection_node) },
		{ UFBX_ELEMENT_CHARACTER, sizeof(ufbx_character) },
		{ UFBX_ELEMENT_CONSTRAINT, sizeof(ufbx_constraint) },
		{ UFBX_ELEMENT_AUDIO_LAYER, sizeof(ufbx_audio_layer) },
		{ UFBX_ELEMENT_AUDIO_CLIP, sizeof(ufbx_audio_clip) },
		{ UFBX_ELEMENT_POSE, sizeof(ufbx_pose) },
		{ UFBX_ELEMENT_METADATA_OBJECT, sizeof(ufbx_metadata_object) },
	};
	ufbxt_assert(ufbxt_arraycount(element_type_sizes) == UFBX_ELEMENT_TYPE_COUNT);
	ufbxt_assert(ufbxt_arraycount(ufbx_element_type_size) == UFBX_ELEMENT_TYPE_COUNT);

	for (size_t i = 0; i < ufbxt_arraycount(element_type_sizes); i++) {
		ufbxt_element_type_size ref = element_type_sizes[i];
		ufbxt_assert(ufbx_element_type_size[(size_t)ref.type] == ref.size);
	}
}
#endif

UFBXT_TEST(evaluate_anim_null)
#if UFBXT_IMPL
{
	ufbx_real r = ufbx_evaluate_anim_value_real(NULL, 0.0);
	ufbxt_assert(r == 0.0f);

	ufbx_vec3 v = ufbx_evaluate_anim_value_vec3(NULL, 0.0);
	ufbxt_assert(v.x == 0.0f && v.y == 0.0f && v.z == 0.0f);
}
#endif

UFBXT_TEST(catch_triangulate)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_node *node = ufbx_find_node(scene, "pCube1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbx_face face = mesh->faces.data[0];
		ufbxt_assert(face.index_begin == 0);
		ufbxt_assert(face.num_indices == 4);

		ufbx_panic panic;

		{
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, face);
			ufbxt_assert(num_tris == 2);
			ufbxt_assert(!panic.did_panic);
		}

		{
			uint32_t indices[4];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 4, mesh, face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face needs at least 6 indices for triangles, got space for 4"));
		}

		{
			ufbx_face bad_face = { 100, 4 };
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, bad_face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index begin (100) out of bounds (24)"));
		}

		{
			ufbx_face bad_face = { 22, 4 };
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, bad_face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index end (22 + 4) out of bounds (24)"));
		}

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(catch_face_normal)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_node *node = ufbx_find_node(scene, "pCube1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbx_face face = mesh->faces.data[0];
		ufbxt_assert(face.index_begin == 0);
		ufbxt_assert(face.num_indices == 4);

		ufbx_panic panic;

		{
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, face);
			ufbxt_assert(!panic.did_panic);
		}

		{
			ufbx_face bad_face = { 100, 4 };
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, bad_face);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index begin (100) out of bounds (24)"));
		}

		{
			ufbx_face bad_face = { 22, 4 };
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, bad_face);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index end (22 + 4) out of bounds (24)"));
		}

		ufbx_free_scene(scene);
	}
}
#endif
