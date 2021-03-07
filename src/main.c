#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <signal.h>

#include "hexGrid.h"
#include "render.h"
#include "chunk_cache.h"
#include "intdef.h"

#include "world.h"

#include "world_def/world_def.h"
#include "world_def/shape.h"
#include "world_def/lexer.h"
#include "world_def/jobs.h"
#include "world_def/path.h"

static bool shouldQuit = false;

void
signalHandler(int sig)
{
	switch (sig) {
		case SIGINT:
			shouldQuit = true;
			break;

		default:
			break;
	}
}

static void glfwErrorCallback(int error, const char *msg) {
	fprintf(stderr, "glfw %i: %s\n", error, msg);
}

typedef struct {
	struct render_context render;
} WindowContext;

void windowSizeCallback(GLFWwindow *win, int width, int height) {
	WindowContext *winCtx =
		(WindowContext *)glfwGetWindowUserPointer(win);

	winCtx->render.screenSize.x = width;
	winCtx->render.screenSize.y = height;

	glViewport(0, 0, width, height);
}

int main(int argc, char *argv[])
{
	int err;

	if (!glfwInit()) {
		printf("Failed to initialize glfw.\n");
		return -1;
	}

	hexGridInitialize();

	struct mgc_memory memory = {0};
	mgc_memory_init(&memory);

	struct arena arena = {0};
	arena_init(&arena, &memory);

	struct arena transient = {0};
	arena_init(&transient, &memory);

	struct mgc_error_context err_ctx = {0};
	err_ctx.string_arena = &arena;
	err_ctx.transient_arena = &transient;

	struct arena atom_arena = {0};
	arena_init(&atom_arena, &memory);

	struct atom_table atom_table = {0};
	atom_table.string_arena = &atom_arena;
	atom_table_rehash(&atom_table, 64);

	struct chunk_gen_mesh_buffer *mesh_gen_mem;
	mesh_gen_mem = arena_alloc(&arena, sizeof(struct chunk_gen_mesh_buffer));

	struct mgc_world_init_context world_init_context = {0};
	world_init_context.atom_table = &atom_table;
	world_init_context.memory = &memory;
	world_init_context.world_arena = &arena;
	world_init_context.transient_arena = &transient;
	world_init_context.err = &err_ctx;

	struct mgc_world world = {0};
	mgc_world_init_world_def(&world, &world_init_context);


	/*
	struct mgcd_context world_decl_ctx = {0};
	mgcd_context_init(
			&world_decl_ctx,
			&atom_table,
			&memory,
			&arena,
			&transient,
			&err_ctx);

	mgcd_resource_id test_shape_id;
	test_shape_id = mgcd_request_resource_str(
			&world_decl_ctx, MGCD_RESOURCE_NONE, STR("/world/test"));

	err = mgcd_jobs_dispatch(&world_decl_ctx);
	print_errors(&err_ctx);

	if (err) {
		return -1;
	}
	assert(err_ctx.num_errors == 0);

	struct mgc_shape *test_shape;
	test_shape = mgcd_expect_shape(&world_decl_ctx, test_shape_id);

	struct mgc_chunk_mask chunk_mask = {0};
	mgc_shape_fill_chunk(
		&chunk_mask,
		(struct mgc_world_transform){0},
		test_shape
	);

	for (size_t i = 0; i < CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT; i++) {
		bool is_set = mgc_chunk_mask_geti(&chunk_mask, i);
		chunk.tiles[i].material = is_set ? (i % 2 == 0 ? MAT_WOOD : MAT_WATER) : MAT_AIR;
	}

	struct mgc_mesh chunkMesh = chunk_gen_mesh(mesh_gen_mem, &mat_table, &chunk);

	// struct mgc_chunk chunk = {0};

	// for (size_t i = 0; i < CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT; i++) {
	// 	mgc_material_id mat = ((i / (CHUNK_WIDTH*CHUNK_WIDTH)) % 2) ? MAT_WOOD : MAT_WATER;
	// 	chunk.tiles[i].material = (tick & (1 << (i % 64))) ? mat : MAT_AIR;
	// }

	// struct mgc_mesh chunkMesh = chunk_gen_mesh(&mat_table, &chunk);
	*/

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	WindowContext winCtx = {0};
	winCtx.render.screenSize.x = 800;
	winCtx.render.screenSize.y = 800;

	GLFWwindow *win;
	win = glfwCreateWindow(
			winCtx.render.screenSize.x,
			winCtx.render.screenSize.y,
			"Magic", NULL, NULL);
	if (!win) {
		fprintf(stderr, "Could not create window.\n");
		return -1;
	}

	glfwSetWindowUserPointer(win, &winCtx);

	glfwSetWindowSizeCallback(win, windowSizeCallback);
	glfwMakeContextCurrent(win);
	gladLoadGL();

	printf("OpenGL %s\n", glGetString(GL_VERSION));

	err = render_initialize(&winCtx.render);
	if (err) {
		return -1;
	}

	struct mgc_material_table mat_table = {0};
	initMaterialTable(&mat_table);
	winCtx.render.materials = &mat_table;

	Camera cam = {0};

	cam.zoom = 0.5f;
	cam.location = V3(2.0f, 2.0f, 2.0f);

	GLuint defaultVShader, defaultFShader;
	defaultVShader = shader_compile_from_file("assets/shaders/default.vsh", GL_VERTEX_SHADER);
	defaultFShader = shader_compile_from_file("assets/shaders/default.fsh", GL_FRAGMENT_SHADER);

	GLuint defaultShader;
	defaultShader = glCreateProgram();

	glAttachShader(defaultShader, defaultVShader);
	glAttachShader(defaultShader, defaultFShader);
	if (!shader_link(defaultShader)) {
		return -1;
	}

	GLint inCameraTransform, inWorldTransform, inNormalTransform, inColor, inLightPos;
	inCameraTransform = glGetUniformLocation(defaultShader, "cameraTransform");
	inNormalTransform = glGetUniformLocation(defaultShader, "normalTransform");
	inWorldTransform = glGetUniformLocation(defaultShader, "worldTransform");
	inColor = glGetUniformLocation(defaultShader, "inColor");
	inLightPos = glGetUniformLocation(defaultShader, "inLightPos");

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	glEnable(GL_DEPTH_TEST);

	signal(SIGINT, signalHandler);

	int tick = 0;

	struct mgc_chunk_cache chunk_cache = {0};
	mgc_chunk_cache_init(&chunk_cache, &world, &mat_table);

	mgc_chunk_cache_request(&chunk_cache, V3i(0, 0, 0));
	mgc_chunk_cache_request(&chunk_cache, V3i(0, 1, 0));
	mgc_chunk_cache_request(&chunk_cache, V3i(0, -1, 0));
	mgc_chunk_cache_request(&chunk_cache, V3i(1, 0, 0));
	mgc_chunk_cache_request(&chunk_cache, V3i(-1, 0, 0));

	struct mgc_chunk_render_entry *render_queue;
	size_t render_queue_cap = 1024;

	render_queue = calloc(sizeof(struct mgc_chunk_render_entry), render_queue_cap);

	while (!glfwWindowShouldClose(win) && !shouldQuit) {
		tick += 1;

		mgc_chunk_cache_tick(&chunk_cache);

		size_t render_queue_length = 0;
		mgc_chunk_cache_make_render_queue(
			&chunk_cache,
			render_queue_cap,
			render_queue,
			&render_queue_length
		);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int width, height;
		glfwGetWindowSize(win, &width, &height);

		m4 perspective;
		mat4_identity(perspective.m);
		mat4_perspective(
				perspective.m, to_radians(90.0f),
				(float)width / (float)height,
				0.1f, 100.0f);

		v3 chunkCenter = V3(
			// 0.0f, 0.0f, 0.0f
			8.0f * hexStrideX + 8.0f * hexStaggerX,
			0,
			8.0f * hexStrideY
		);

		cam.location = v3_add(chunkCenter, V3(
			0.0f, 8.0f, 10.0f
			// cos(((float)tick / 480.0f) * 2 * M_PI) * 10.0f,
			// 8.0f,
			// sin(((float)tick / 480.0f) * 2 * M_PI) * 10.0f
		));

		v3 lightPos = v3_add(chunkCenter, V3(
			cos(((float)tick / 480.0f) * 2 * M_PI) * 10.0f,
			8.0f,
			sin(((float)tick / 480.0f) * 2 * M_PI) * 10.0f
		));

		v3 lookAt = chunkCenter;
		v3 up = V3(0.0f, 1.0f, 0.0f);
		m4 camera;
		mat4_look_at(camera.m, cam.location.m, lookAt.m, up.m);

		m4 cameraTransform;
		mat4_multiply(cameraTransform.m, perspective.m, camera.m);

		glUseProgram(defaultShader);

		m4 normalTransform;
		mat4_identity(normalTransform.m);
		glUniformMatrix4fv(inCameraTransform, 1, GL_TRUE, cameraTransform.m);
		glUniformMatrix4fv(inNormalTransform, 1, GL_TRUE, normalTransform.m);

		for (size_t queue_i = 0; queue_i < render_queue_length; queue_i++) {
			struct mgc_chunk_render_entry *entry = &render_queue[queue_i];
			v3i chunk_coord = entry->coord;
			chunk_coord.x *= CHUNK_WIDTH;
			chunk_coord.y *= CHUNK_HEIGHT;
			chunk_coord.z *= CHUNK_WIDTH;
			v3 chunk_location = mgc_grid_draw_coord(chunk_coord);

			glBindVertexArray(entry->mesh.vao);

			m4 worldTransform;
			mat4_identity(worldTransform.m);
			mat4_translate(worldTransform.m, worldTransform.m, chunk_location.m);

			glUniformMatrix4fv(inWorldTransform,  1, GL_TRUE, worldTransform.m);
			glUniform3fv(inLightPos, 1, lightPos.m);

			glUniform3f(inColor, 1.0f, 1.0f, 1.0f);

			// glLineWidth(1.0f);
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glDrawArrays(GL_TRIANGLES, 0, entry->mesh.numVertices);
		}

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	signal(SIGINT, SIG_DFL);

	free(render_queue);
	free(chunk_cache.entries);
	atom_table_destroy(&atom_table);
	mgc_memory_destroy(&memory);
}
