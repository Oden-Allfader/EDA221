#include "assignment5.hpp"
#include "interpolation.hpp"
#include "node.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

eda221::Assignment5::Assignment5()
{
	Log::View::Init();

	window = Window::Create("EDA221: Assignment 5", config::resolution_x,
		config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}


eda221::Assignment5::~Assignment5()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}
void eda221::Assignment5::run()
{
	// Loading the level geometry
	// Water that goes to the edge of the skybox
	auto water_shape = parametric_shapes::createQuad(100u, 100u, 500u, 500u);
	if (water_shape.vao == 0u) {
		LogError("Failed to retrive quad mesh");
		return;
	}
	// Loading the skybox geometry
	auto skybox_shape = parametric_shapes::createSphere(50u, 50u, 300.0f);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the circle ring mesh");
		return;
	}
	auto snake_shape = parametric_shapes::createSphere(10,10,1.0f);
	// Set up the camera 
	// Get the camera to follow the snake.
	// Maybe start by having the camera looking down on the map to see if it works,
	// We can add two camera modes perhaps.
	FPSCameraf mCamera(bonobo::pi / 4.0f,
		static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
		0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	// Water shader for the level
	auto water_shader = eda221::createProgram("water.vert", "water.frag");
	if (water_shader == 0u) {
		LogError("Failed to load water shader");
		return;
	}
	// Skybox shader for the skybox
	auto skybox_shader = eda221::createProgram("skybox.vert", "skybox.frag");
	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}
	// Shader for the snake head
	auto snake_head_shader = eda221::createProgram("diffuse.vert", "diffuse.frag");
	if (snake_head_shader == 0u) {
		LogError("Failed to load snake head shader");
		return;
	}

	// Initializing the time variables.
	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	// Set the uniforms of the shaders
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f); // Light position;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const set_uniforms = [&light_position, &camera_position, &nowTime](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time"), static_cast<float>(nowTime) / 1000.0f);
	};
	auto polygon_mode = polygon_mode_t::fill;

	auto bumpTex = loadTexture2D("lavanormal.png");
	std::string cubeName = "Lava/";
	auto cloud = loadTextureCubeMap(cubeName + "posx.png", cubeName + "negx.png", cubeName + "posy.png", cubeName + "negy.png", cubeName + "posz.png", cubeName + "negz.png", true);

	auto water_quad = Node();
	water_quad.set_geometry(water_shape);
	water_quad.set_program(water_shader, set_uniforms);
	water_quad.add_texture("bumpTex", bumpTex);
	water_quad.add_texture("cubeTex", cloud, GL_TEXTURE_CUBE_MAP);

	auto skybox = Node();
	skybox.set_geometry(skybox_shape);
	skybox.set_program(skybox_shader,set_uniforms);
	skybox.add_texture("cubeMapName", cloud , GL_TEXTURE_CUBE_MAP);

	auto snake_head = Node();
	snake_head.set_geometry(snake_shape);
	snake_head.set_program(snake_head_shader,set_uniforms);

	glEnable(GL_DEPTH_TEST);
	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	//Initializing positions.
	water_quad.translate(glm::vec3(-150.0f,-1.5f,-150.0f));
	snake_head.set_translation(glm::vec3(0.0f,0.0f,0.0f));
	float turning = 0.0f;
	float speed = 20.0f;
	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		switch (polygon_mode) {
		case polygon_mode_t::fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case polygon_mode_t::line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case polygon_mode_t::point:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		}


		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & PRESSED) {
			turning = fmod(turning + 0.1, bonobo::two_pi);
		}else if(inputHandler->GetKeycodeState(GLFW_KEY_2) & PRESSED) {
			turning = fmod(turning - 0.1, bonobo::two_pi);
		}

		snake_head.translate(glm::vec3(speed*ddeltatime/1000 * cos(turning), 0.0f, -speed*ddeltatime/1000 * sin(turning)));


		//mCamera.mWorld.LookAt(glm::vec3());
		camera_position = mCamera.mWorld.GetTranslation();
		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		water_quad.render(mCamera.GetWorldToClipMatrix(), water_quad.get_transform());
		skybox.render(mCamera.GetWorldToClipMatrix(), skybox.get_transform());
		snake_head.render(mCamera.GetWorldToClipMatrix(),snake_head.get_transform());
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			//			ImGui::SliderInt("Faces Nb", &faces_nb, 1u, 16u);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms, %.1f fps  %.1f turning", ddeltatime, 1000 / (ddeltatime),turning);
		ImGui::End();

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}
	glDeleteProgram(water_shader);
	water_shader = 0u;
	glDeleteProgram(skybox_shader);
	skybox_shader = 0u;
	glDeleteProgram(snake_head_shader);
	snake_head_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		eda221::Assignment5 assignment5;
		assignment5.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}

