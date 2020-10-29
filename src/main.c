#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <signal.h>

#include "hexGrid.h"
#include "render.h"
#include "intdef.h"

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
	if (!glfwInit()) {
		printf("Failed to initialize glfw.\n");
		return -1;
	}

	hexGridInitialize();

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

	int err;
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
	cam.location.x = 8.0f;
	cam.location.y = 8.0f;

	for (size_t i = 0; i < CHUNK_WIDTH*CHUNK_WIDTH; i++) {
		chunk.tiles[i].material = MAT_WATER;
	}

	/*
	for (size_t i = 0; i < CHUNK_WIDTH*CHUNK_WIDTH; i++) {
		chunk.tiles[i] = 
			tileTransition(&materialTable, &chunk.tiles[i], NULL, 0);
	}
	*/

	signal(SIGINT, signalHandler);

	while (!glfwWindowShouldClose(win) && !shouldQuit) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderChunk(&winCtx.render, cam, &chunk);

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	signal(SIGINT, SIG_DFL);
}
