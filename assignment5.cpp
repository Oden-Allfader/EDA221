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

bool testSphereSphere(glm::vec3 const&p1, float const r1, glm::vec3 const&p2, float const r2) {
	return glm::length(p1 - p2) < (r1 + r2);
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
		LogError("Failed to retrieve the sphere mesh");
		return;
	}
	auto const head_shape = eda221::loadObjects("ogre.obj");
	if (head_shape.empty()) {
		LogError("Failed to load head object");
		return;
	}

	float snake_radius = 2.0f;
	auto snake_shape = parametric_shapes::createSphere(10, 10, snake_radius);
	if (snake_shape.vao == 0u) {
		LogError("Failed to retrieve the snake mesh");
		return;
	}
	float boundry_radius = 5.0f;
	auto boundry_shape = parametric_shapes::createSphere(10, 10, boundry_radius);
	if (boundry_shape.vao == 0u) {
		LogError("Failed to retrieve the boundry mesh");
		return;
	}
	auto tetra = eda221::loadObjects("stone.obj");
	if (tetra.empty()) {
		LogError("Failed to load head object");
		return;
	}
	float food_radius = 1.0f;
	auto food_shape = parametric_shapes::createSphere(10, 10, food_radius);
	if (food_shape.vao == 0u) {
		LogError("Failed to retrieve the food mesh");
		return;
	}


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
	// Shader for the boundry
	auto boundry_shader = eda221::createProgram("diffuse.vert", "diffuse.frag");
	if (boundry_shader == 0u) {
		LogError("Failed to load snake head shader");
		return;
	}
	// Shader for the food
	auto food_shader = eda221::createProgram("diffuse.vert", "diffuse.frag");
	if (food_shader == 0u) {
		LogError("Failed to load food shader");
		return;
	}

	// Initializing the time variables.
	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;



	// Set the uniforms of the shaders
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f); // Light position;
	mCamera.mWorld.SetTranslate(glm::vec3(-100.0f, 100.0f, -100.0f));
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

	//Level Node
	auto water_quad = Node();
	water_quad.set_geometry(water_shape);
	water_quad.set_program(water_shader, set_uniforms);
	water_quad.add_texture("bumpTex", bumpTex);
	water_quad.add_texture("cubeTex", cloud, GL_TEXTURE_CUBE_MAP);
	//Skybox Node
	auto skybox = Node();
	skybox.set_geometry(skybox_shape);
	skybox.set_program(skybox_shader, set_uniforms);
	skybox.add_texture("cubeMapName", cloud, GL_TEXTURE_CUBE_MAP);
	//Snake head Node
	auto snake_head = Node();
	snake_head.set_geometry(snake_shape);
	snake_head.set_program(snake_head_shader, set_uniforms);
	auto snake_head_t = Node();
	snake_head_t.set_geometry(head_shape.at(0));
	snake_head_t.set_program(snake_head_shader,set_uniforms);
	snake_head.add_child(&snake_head_t);
	snake_head.scale(glm::vec3(2, 2, 2));

	//Snake body nodes
	int const no_snake = 100;
	Node snake_bodies[no_snake];
	for (int i = 0; i < no_snake; i++) {
		snake_bodies[i].set_geometry(snake_shape);
		snake_bodies[i].set_program(snake_head_shader, set_uniforms);
	}
	//Food node
	auto food = Node();
	food.set_geometry(food_shape);
	food.set_program(food_shader,set_uniforms);

	//Creation of boundry nodes.
	int const no_boundries = 100;
	Node boundries[no_boundries];
	Node tetras[no_boundries];
	glm::vec3 boundry_pos[no_boundries];
	for (int i = 0; i < no_boundries; i++) {
		boundries[i].set_geometry(boundry_shape);
		boundries[i].set_program(boundry_shader, set_uniforms);
		boundries[i].add_child(&tetras[i]);
		tetras[i].set_geometry(tetra.at(0));
		tetras[i].set_program(boundry_shader,set_uniforms);
	}
	for (int i = 0; i < 25; i++) {
		boundries[i].set_translation(glm::vec3(-100.0f, 0.0f, 100.0f - 8 * i));
		boundry_pos[i] = glm::vec3(-100.0f, 0.0f, 100.0f - 8 * i);

		boundries[i + 25].set_translation(glm::vec3(100.0f, 0.0f, 8 * i - 100.0f));
		boundry_pos[i + 25] = glm::vec3(100.0f, 0.0f, 8 * i - 100.0f);

		boundries[i + 50].set_translation(glm::vec3(-100.0f + 8 * i, 0.0f, -100.0f));
		boundry_pos[i + 50] = glm::vec3(-100.0f + 8 * i, 0.0f, -100.0f);

		boundries[i + 75].set_translation(glm::vec3(100.0f - 8 * i, 0.0f, 100.0f));
		boundry_pos[i + 75] = glm::vec3(100.0f - 8 * i, 0.0f, 100.0f);
	}

	glm::vec3 cp[no_snake]; // no_snake control points
	for (int i = 0; i < no_snake; i++) {
		cp[i] = glm::vec3(0,0,0);
	}


	glEnable(GL_DEPTH_TEST);
	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	//Initializing positions.
	water_quad.translate(glm::vec3(-250.0f, -1.5f, -250.0f)); // Moving lava down
	snake_head.set_translation(glm::vec3(0.0f, 0.0f, 0.0f)); // centering snake head:
	float turning = 0.0f; //init angle
	float speed = 25.0f; // init speed
	glm::vec3 snake_pos = glm::vec3(0.0f, 0.0f, 0.0f); //init snake pos
	bool crashed = false;//game losing bool
	glm::vec3 food_pos = glm::vec3(0, 0, 0); //Calculating pos of food
	while (abs(snake_pos.x - food_pos.x) < 2 && abs(snake_pos.z - food_pos.z) < 2) {
		food_pos.x = rand() % 180 - 90;
		food_pos.z = rand() % 180 - 90;
	}
	food.set_translation(food_pos);
	int count = 0;
	unsigned int score = 0;
	float distance = 0;
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
		mCamera.mWorld.LookAt(glm::vec3(snake_pos.x,snake_pos.y,snake_pos.z));
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
		}
		else if (inputHandler->GetKeycodeState(GLFW_KEY_2) & PRESSED) {
			turning = fmod(turning - 0.1, bonobo::two_pi);
		}

		snake_head.translate(glm::vec3(speed*ddeltatime / 1000 * cos(turning), 0.0f, -speed*ddeltatime / 1000 * sin(turning)));
		snake_head.set_rotation_y(turning + bonobo::pi/2);
		snake_pos = glm::vec3(snake_pos.x + speed*ddeltatime / 1000 * cos(turning), 0, snake_pos.z - speed*ddeltatime / 1000 * sin(turning));

		//if ((int)(nowTime/1000) % (int)(2 * snake_radius / (speed*ddeltatime / 1000)) == 0) {
		if (distance > 2*snake_radius) {
			for (int i = no_snake-1; i > 0; i--) {
				cp[i] = cp[i - 1];
			}
			distance = 0;
			cp[0] = snake_pos;
		}
		for (int i = 0; i < no_snake; i++) {
			snake_bodies[i].set_translation(cp[i]);
		}



		// Testing collision
		for (int i = 0; i < no_boundries; i++) {
			if (testSphereSphere(snake_pos, snake_radius, boundry_pos[i], boundry_radius)) {
				snake_head.translate(glm::vec3(-snake_pos.x, 0, -snake_pos.z));
				snake_pos = glm::vec3(0.0f, 0.0f, 0.0f);
				score = 0;
			}
		}
		for (int i = 2; i < score; i++) {
			if (testSphereSphere(snake_pos, snake_radius, cp[i], snake_radius)) {
				snake_head.translate(glm::vec3(-snake_pos.x, 0, -snake_pos.z));
				snake_pos = glm::vec3(0.0f, 0.0f, 0.0f);
				score = 0;
			}
		}
		if (testSphereSphere(snake_pos, snake_radius, food_pos, food_radius)) {
			score++;
			while (abs(snake_pos.x - food_pos.x) < 2 || abs(snake_pos.z - food_pos.z) < 2) {
				food_pos.x = rand() % 190 - 95;
				food_pos.z = rand() % 190 - 95;
			}
			food.set_translation(food_pos);
		}
		
		//mCamera.mWorld.LookAt(glm::vec3());
		camera_position = mCamera.mWorld.GetTranslation();
		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		water_quad.render(mCamera.GetWorldToClipMatrix(), water_quad.get_transform());
		skybox.render(mCamera.GetWorldToClipMatrix(), skybox.get_transform());
		snake_head_t.render(mCamera.GetWorldToClipMatrix(), snake_head.get_transform());
		food.render(mCamera.GetWorldToClipMatrix(), food.get_transform());
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		for (int i = 0; i < no_boundries; i++) {
			boundries[i].render(mCamera.GetWorldToClipMatrix(), boundries[i].get_transform());
		}
		for (int i = 0; i < score + 1 && i < no_snake; i++) {
			snake_bodies[i].render(mCamera.GetWorldToClipMatrix(), snake_bodies[i].get_transform());
		}
		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			//			ImGui::SliderInt("Faces Nb", &faces_nb, 1u, 16u);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms, %.1f fps  %.1f turning \n Position of snake: x: %f, y: %f, z: %f , Score: %d", ddeltatime, 1000 / (ddeltatime), turning, snake_pos[0], snake_pos[1], snake_pos[2],score);
		ImGui::End();

		ImGui::Render();
		window->Swap();
		lastTime = nowTime;

		distance = distance + sqrt(pow(speed*ddeltatime / 1000 * cos(turning), 2) + pow(speed*ddeltatime / 1000 * sin(turning), 2));
		
		

		count++;
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

