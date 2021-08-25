#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <signal.h>

#include "profile.h"

#include "hexGrid.h"
#include "render.h"
#include "chunk_cache.h"
#include "intdef.h"

#include "world.h"
#include "registry.h"
#include "sim.h"
#include "sim_thread.h"

#include "wd_world_def.h"
#include "wd_shape.h"
#include "wd_lexer.h"
#include "wd_jobs.h"
#include "wd_path.h"

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

static void
glfw_error_callback(int error, const char *msg) {
	fprintf(stderr, "glfw %i: %s\n", error, msg);
}

typedef struct {
	struct render_context render;

	bool enable_mouse_input;
	struct {
		bool left;
		bool right;
		bool up;
		bool down;
		bool forward;
		bool backward;
	} keys;
} WindowContext;

struct mgc_camera {
	v3 position;
	float yaw, pitch;
};

void windowSizeCallback(GLFWwindow *win, int width, int height) {
	WindowContext *win_ctx =
		(WindowContext *)glfwGetWindowUserPointer(win);

	win_ctx->render.screenSize.x = width;
	win_ctx->render.screenSize.y = height;

	glViewport(0, 0, width, height);
}

static void
mouse_button_callback(GLFWwindow *win, int button, int action, int mods)
{
	WindowContext *win_ctx =
		(WindowContext *)glfwGetWindowUserPointer(win);
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		win_ctx->enable_mouse_input = true;
	}
}

static void
key_button_callback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
	WindowContext *win_ctx =
		(WindowContext *)glfwGetWindowUserPointer(win);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		win_ctx->enable_mouse_input = false;
	}

#define KEYDEF(bind_name, key_id) \
	if (key == key_id) { \
		if (action == GLFW_PRESS)   win_ctx->keys.bind_name = true;  \
		if (action == GLFW_RELEASE) win_ctx->keys.bind_name = false; \
	}

	KEYDEF(left, GLFW_KEY_A);
	KEYDEF(right, GLFW_KEY_D);
	KEYDEF(forward, GLFW_KEY_W);
	KEYDEF(backward, GLFW_KEY_S);
	KEYDEF(up, GLFW_KEY_E);
	KEYDEF(down, GLFW_KEY_Q);

#undef KEYDEF
}

static void
cursor_enter_callback(GLFWwindow *win, int entered)
{
	WindowContext *win_ctx =
		(WindowContext *)glfwGetWindowUserPointer(win);
	memset(&win_ctx->keys, 0, sizeof(win_ctx->keys));
}

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int showCmd)
#else
int main(int argc, char *argv[])
#endif
{
	int err;

#ifdef _WIN32
#if 0
	AllocConsole();
	FILE *console_fp;
	freopen_s(&console_fp, "CONIN$", "r", stdin);
	freopen_s(&console_fp, "CONOUT$", "w", stdout);
	freopen_s(&console_fp, "CONOUT$", "w", stderr);
#endif
#endif

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

	struct mgc_registry reg = {0};
	mgc_material_table_init(&reg.materials);

	struct mgc_world_init_context world_init_context = {0};
	world_init_context.atom_table = &atom_table;
	world_init_context.memory = &memory;
	world_init_context.world_arena = &arena;
	world_init_context.transient_arena = &transient;
	world_init_context.registry = &reg;
	world_init_context.err = &err_ctx;

	struct mgc_world world = {0};
	mgc_world_init_world_def(&world, &world_init_context);

	struct mgc_chunk_cache chunk_cache = {0};
	mgc_chunk_cache_init(&chunk_cache, &memory, &world, &reg.materials);

	glfwSetErrorCallback(glfw_error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	WindowContext win_ctx = {0};
	win_ctx.render.screenSize.x = 800;
	win_ctx.render.screenSize.y = 800;
	win_ctx.render.materials = &reg.materials;

	GLFWwindow *win;
	win = glfwCreateWindow(
			win_ctx.render.screenSize.x,
			win_ctx.render.screenSize.y,
			"Magic", NULL, NULL);
	if (!win) {
		fprintf(stderr, "Could not create window.\n");
		return -1;
	}

	glfwSetWindowUserPointer(win, &win_ctx);

	glfwSetWindowSizeCallback(win, windowSizeCallback);
	glfwSetMouseButtonCallback(win, mouse_button_callback);
	glfwSetKeyCallback(win, key_button_callback);
	glfwSetCursorEnterCallback(win, cursor_enter_callback);

	glfwMakeContextCurrent(win);
	gladLoadGL();

	printf("OpenGL %s\n", glGetString(GL_VERSION));

	err = render_initialize(&win_ctx.render);
	if (err) {
		return -1;
	}

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

	struct mgc_sim_thread sim_thread_info = {0};
	mgc_sim_thread_start(&sim_thread_info, &arena, &chunk_cache, &reg);

	int tick = 0;


	// struct mgc_sim_buffer *sim_buffer = calloc(sizeof(struct mgc_sim_buffer), 1);

	struct mgc_chunk_render_entry *render_queue;
	size_t render_queue_cap = 1024;

	render_queue = calloc(sizeof(struct mgc_chunk_render_entry), render_queue_cap);

	bool mouse_control_enabled = false;

	double last_cur_x = 0, last_cur_y = 0;
	struct mgc_camera c = {0};
	// c.position.x = 16.0f;
	// c.position.y = 16.0f;
	c.position.z = 16.0f;
	c.pitch = -.25;
	c.yaw = .75;

	signal(SIGINT, signalHandler);
	while (!glfwWindowShouldClose(win) && !shouldQuit) {
		tick += 1;

		if (!mouse_control_enabled && win_ctx.enable_mouse_input) {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (glfwRawMouseMotionSupported()) {
				glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
			}

			mouse_control_enabled = true;
			glfwGetCursorPos(win, &last_cur_x, &last_cur_y);
		} else if (mouse_control_enabled && !win_ctx.enable_mouse_input) {
			if (glfwRawMouseMotionSupported()) {
				glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
			}
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			mouse_control_enabled = false;
		}

		if (mouse_control_enabled) {
			double cur_x, cur_y;
			glfwGetCursorPos(win, &cur_x, &cur_y);
			v2 delta = V2(cur_x - last_cur_x, cur_y - last_cur_y);

			last_cur_x = cur_x;
			last_cur_y = cur_y;

			c.yaw += (float)delta.x * 0.005f;
			c.yaw -= floor(c.yaw);

			c.pitch -= (float)delta.y * 0.005f;
			if (c.pitch > 1.0f) {
				c.pitch = 1.0f;
			} else if (c.pitch < -1.0f) {
				c.pitch = -1.0f;
			}

			v3 move_delta = {0};
			v3 move_final = {0};

			if (win_ctx.keys.left)     {move_delta.x -= 1.0f;}
			if (win_ctx.keys.right)    {move_delta.x += 1.0f;}

			if (win_ctx.keys.up)       {move_delta.y += 1.0f;}
			if (win_ctx.keys.down)     {move_delta.y -= 1.0f;}

			if (win_ctx.keys.forward)  {move_delta.z -= 1.0f;}
			if (win_ctx.keys.backward) {move_delta.z += 1.0f;}

			if (move_delta.x != 0.0f) {
				move_final.x += cos(PI*2.0f*c.yaw)/move_delta.x;
				move_final.z += sin(PI*2.0f*c.yaw)/move_delta.x;
			}
			if (move_delta.z != 0.0f) {
				float pitch_comp = fabs(cos((PI/2.0f)*c.pitch));
				move_final.x += pitch_comp*-sin(PI*2.0f*c.yaw)/move_delta.z;
				move_final.z += pitch_comp*cos(PI*2.0f*c.yaw)/move_delta.z;
				move_final.y += -sin((PI/2.0f)*c.pitch)/move_delta.z;
			}

			move_final.y += move_delta.y;

			float accel = 0.5f;

			move_final.x *= accel;
			move_final.y *= accel;
			move_final.z *= accel;

			c.position = v3_add(c.position, move_final);
		}

		if (tick % 30 == 0) {
			v3i sim_center = V3i(0, 0, 0);
			mgc_chunk_cache_set_sim_center(&chunk_cache, sim_center);
			mgc_chunk_cache_tick(&chunk_cache);
		}


		mgc_chunk_cache_render_tick(&chunk_cache);

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

		v3 lightPos = v3_add(chunkCenter, V3(
			cos(((float)tick / 480.0f) * 2 * PI) * 50.0f,
			8.0f,
			sin(((float)tick / 480.0f) * 2 * PI) * 50.0f
		));

		m4 camera, cam_tmp;

		mat4_identity(camera.m);

		v3 axis_x = V3(1.0f, 0.0f, 0.0f);
		v3 axis_y = V3(0.0f, 1.0f, 0.0f);

		mat4_identity(cam_tmp.m);
		// mat4_rotation_x(cam_tmp.m, PI/2 * c.pitch);
		mat4_rotation_axis(cam_tmp.m, axis_x.m, (PI/2.0f) * -c.pitch);
		mat4_multiply(camera.m, camera.m, cam_tmp.m);

		mat4_identity(cam_tmp.m);
		// mat4_rotation_y(cam_tmp.m, PI*2 * c.yaw);
		mat4_rotation_axis(cam_tmp.m, axis_y.m, PI*2.0f * c.yaw);
		mat4_multiply(camera.m, camera.m, cam_tmp.m);

		v3 cam_pos = V3(-c.position.x, -c.position.y, -c.position.z);
		mat4_identity(cam_tmp.m);
		mat4_translate(cam_tmp.m, cam_tmp.m, cam_pos.m);
		mat4_multiply(camera.m, camera.m, cam_tmp.m);

		m4 cameraTransform;
		mat4_multiply(cameraTransform.m, perspective.m, camera.m);

		glUseProgram(defaultShader);

		m4 normalTransform;
		mat4_identity(normalTransform.m);
		glUniformMatrix4fv(inCameraTransform, 1, GL_TRUE, cameraTransform.m);
		glUniformMatrix4fv(inNormalTransform, 1, GL_TRUE, normalTransform.m);

		for (size_t queue_i = 0; queue_i < render_queue_length; queue_i++) {
			struct mgc_chunk_render_entry *entry = &render_queue[queue_i];
			v3 chunk_location = mgc_grid_draw_coord(entry->coord);

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

		TracyCFrameMark;

		glfwPollEvents();
	}

	signal(SIGINT, SIG_DFL);

	mgc_sim_thread_stop(&sim_thread_info);

	free(render_queue);
	free(chunk_cache.entries);
	atom_table_destroy(&atom_table);
	mgc_memory_destroy(&memory);

	return 0;
}
