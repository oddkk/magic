#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <signal.h>

#include "hexGrid.h"
#include "render.h"
#include "intdef.h"

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
	RenderContext render;
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

	struct arena tmp_arena = {0};
	arena_init(&tmp_arena, &memory);

	struct mgc_error_context err_ctx = {0};
	err_ctx.string_arena = &arena;
	err_ctx.transient_arena = &tmp_arena;

	struct arena atom_arena = {0};
	arena_init(&atom_arena, &memory);

	struct atom_table atom_table = {0};
	atom_table.string_arena = &atom_arena;
	atom_table_rehash(&atom_table, 64);

	struct mgcd_world world = {0};
	mgcd_world_init(&world, &memory);

	struct mgcd_context world_decl_ctx = {0};
	mgcd_context_init(
			&world_decl_ctx,
			&world,
			&atom_table,
			&memory,
			&arena,
			&tmp_arena,
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

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	WindowContext winCtx = {0};
	winCtx.render.screenSize.x  = 800;
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

	err = renderInitialize(&winCtx.render);
	if (err) {
		return -1;
	}

	MaterialTable materialTable = {0};
	initMaterialTable(&materialTable);
	winCtx.render.materials = &materialTable;

	Chunk chunk = {0};
	Camera cam = {0};

	cam.zoom = 0.5f;
	cam.location = V3(2.0f, 2.0f, 2.0f);

	for (size_t i = 0; i < CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT; i++) {
		bool is_set = mgc_chunk_mask_geti(&chunk_mask, i);
		chunk.tiles[i].material = is_set ? MAT_WOOD : MAT_AIR;
	}

	GLuint defaultVShader, defaultFShader;
	defaultVShader = compileShaderFromFile("assets/shaders/default.vsh", GL_VERTEX_SHADER);
	defaultFShader = compileShaderFromFile("assets/shaders/default.fsh", GL_FRAGMENT_SHADER);

	GLuint defaultShader;
	defaultShader = glCreateProgram();

	glAttachShader(defaultShader, defaultVShader);
	glAttachShader(defaultShader, defaultFShader);
	if (!linkShaderProgram(defaultShader)) {
		return -1;
	}

	GLint inCameraTransform, inWorldLocation, inColor;
	inCameraTransform = glGetUniformLocation(defaultShader, "cameraTransform");
	inWorldLocation = glGetUniformLocation(defaultShader, "worldLocation");
	inColor = glGetUniformLocation(defaultShader, "inColor");

	Mesh chunkMesh = chunkGenMesh(&materialTable, &chunk);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	glEnable(GL_DEPTH_TEST);

	signal(SIGINT, signalHandler);

	int tick = 0;

	while (!glfwWindowShouldClose(win) && !shouldQuit) {
		tick += 1;
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
			// 0.0f, 8.0f, 10.0f
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
		glBindVertexArray(chunkMesh.vao);

		v3 chunkLocation = V3(
			0.0f,
			0.0f,
			0.0f
		);

		glUniform3fv(inWorldLocation, 1, chunkLocation.m);
		glUniformMatrix4fv(inCameraTransform, 1, GL_TRUE, cameraTransform.m);

		glLineWidth(1.0f);

		glUniform3f(inColor, 1.0f, 1.0f, 1.0f);

		// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glDrawArrays(GL_TRIANGLES, 0, chunkMesh.numVertices);

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	signal(SIGINT, SIG_DFL);

	mgc_memory_destroy(&memory);
}
