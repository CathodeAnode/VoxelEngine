#include <iostream>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>

#include <windows.h>

#include "shader.h"
#include "mouse.h"
#include "keyboard.h"
#include "joystick.h"
#include "camera.h"

#include "screen.h"

#include "chunk.h"
#include "renderer.h"

void processInput(Screen& screen, double dt);

Joystick mainJ(0);

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, -10.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Screen screen(800, 600, "VoxelEngine");


// TODOS:
// 1- Design & Create World class that generates chunks relative to player
//	a- flat world generation
//	b- perlin noise world generation
//	c- saving & loading editied chunk 
//	d- world saving & loading
// 2- Implement chunk editting (removing/adding blocks)
// 3- Implement frustum culling for chunk loading
// 4- Implement binary greedy meshing for chunk rendering
// 5- Optimize adjacent chunk rendering by skipping the rendering of shared borders between neighboring chunks
// 6- Implement 8x8x8 voxel structure for blocks

// less prio:
// 1- ligthing
// 2- ambient occlusion
// 3- collison physics
// 4- otho camera
// 5- some core class instead of global vars & spaghetti code in main
// 6- input manager class
// 7- implement EBO vertiex indicies for renderer
// 8- skybox

// future:
// editor methods:
// building commands:
//	1- tool to select two points fill in with blocks
//	2- select area tool
//	3- copy&paste selcted area tool
//	4- change blocks in selected area tool
//	5- for more tool ideas, see that one minecraft wooden axe mod ...
// misc:
//	1- toggle mesh lines (triangles drawn)
//	2- voxel grid
//	3- file orginization



// Notes:
// use texture atlas to texture large meshes with mutliple voxel textures, https://www.reddit.com/r/VoxelGameDev/comments/10uqgem/how_to_draw_a_mesh_containing_multiple_textures/
// Ambient occlusion: https://www.youtube.com/watch?v=3WaLMBiezMU

int main() {
	Chunk chunk;

	//chunk.printData();

	//chunk.toggleBlock(0, 0, 0);
	//chunk.toggleBlock(7, 7, 7);

	//std::cout << "============================================\n";
	//chunk.printData();


	if (!screen.init()) {
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	screen.enableInputs();
	screen.toggleCursor();

	mainJ.update();
	if (mainJ.isPresent()) {
		std::cout << mainJ.getName() << " is connected.\n";
	}
	else {
		std::cout << "Joystick not connected.\n";
	}

	/*
	-----------------------
		Shaders
	-----------------------
	*/

	Shader shaderPrgm = Shader("core_vertex_shader.glsl", "core_fragment_shader.glsl");

	shaderPrgm.use();

	// vertex array
	float vertices[] = {
		// position			texture cords
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	Renderer renderer(shaderPrgm);

	renderer.bindGPUObjects();

	//std::vector<VertexAttribute> attribs = { {0, 3, GL_FLOAT, GL_FALSE, 0},
	//										 {1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float)} };
	//
	//renderer.uploadVertexData(vertices, 36, 5 * sizeof(float));
	//renderer.setVertexLayout(attribs, 5 * sizeof(float));
	//// VAO, VBO, EBO
	//unsigned int VAO, VBO;
	//glGenVertexArrays(1, &VAO);
	//glGenBuffers(1, &VBO);


	//// bind VAO
	//glBindVertexArray(VAO);

	//// bind VBO
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//// set attribute pointer
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	//glEnableVertexAttribArray(0);

	//// texture attribute pointer
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	//glEnableVertexAttribArray(1);

	// create transformation for screen
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	//glm::vec3 cubePositions[] = {
	//	glm::vec3(0.0f,  0.0f,  0.0f),
	//	glm::vec3(2.0f,  5.0f, -15.0f),
	//	glm::vec3(-1.5f, -2.2f, -2.5f),
	//	glm::vec3(-3.8f, -2.0f, -12.3f),
	//	glm::vec3(2.4f, -0.4f, -3.5f),
	//	glm::vec3(-1.7f,  3.0f, -7.5f),
	//	glm::vec3(1.3f, -2.0f, -2.5f),
	//	glm::vec3(1.5f,  2.0f, -2.5f),
	//	glm::vec3(1.5f,  0.2f, -1.5f),
	//	glm::vec3(-1.3f,  1.0f, -1.5f)
	//};
	double lastToggleTime = 0.0;
	int i = 0;
	while (screen.isOpen()) {
		double currTime = glfwGetTime();
		deltaTime = currTime - lastFrame;
		lastFrame = currTime;

		// process input
		processInput(screen, deltaTime);

		// render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		model = glm::rotate(model, (float)glfwGetTime() * glm::radians(-55.0f), glm::vec3(0.5f));
		//view = glm::translate(view, glm::vec3(-x, -y, -z));
		view = camera.getViewMatrix();
		projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);

		// draw
		shaderPrgm.use();

		shaderPrgm.setMat4("view", view);
		shaderPrgm.setMat4("projection", projection);

		double currentTime = glfwGetTime();

		if (currentTime - lastToggleTime >= 0.02) {
			chunk.data.flip(i);        // Toggle i-th bit
			i = (i + 1) % chunk.data.size();  // Wrap around if needed
			lastToggleTime = currentTime;
		}
		renderer.primitiveChunkRender(chunk, glm::vec3(0, 0, 0));

		//glBindVertexArray(VAO);
		//for (unsigned int i = 0; i < 10; i++)
		//{
		//	//glm::mat4 model = glm::mat4(1.0f);
		//	//model = glm::translate(model, cubePositions[i]);
		//	//float angle = 20.0f * i;
		//	//model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		//	//model = glm::scale(model, glm::vec3(i/10.0f, i/10.0f, i/10.0f));
		//	//shaderPrgm.setMat4("model", model);

		//	//glDrawArrays(GL_TRIANGLES, 0, 36);

		//	renderer.drawVertexMesh(36, cubePositions[i], glm::vec3(i / 10.0f, i / 10.0f, i / 10.0f));
		//}
		///glBindVertexArray(0);

		// send back buffer to front buffer
		screen.update();
	}

	//glDeleteVertexArrays(1, &VAO);
	//glDeleteBuffers(1, &VBO);
	renderer.unbindGPUObjects();

	glfwTerminate();

	return 0;
}

void processInput(Screen& screen, double dt) {
	if (Keyboard::key(GLFW_KEY_ESCAPE)) {
		screen.close();
	}

	if (Keyboard::key(GLFW_KEY_W)) {
		camera.updateCameraPos(CameraDirection::FORWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_S)) {
		camera.updateCameraPos(CameraDirection::BACKWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_D)) {
		camera.updateCameraPos(CameraDirection::RIGHT, dt);
	}

	if (Keyboard::key(GLFW_KEY_A)) {
		camera.updateCameraPos(CameraDirection::LEFT, dt);
	}

	if (Keyboard::key(GLFW_KEY_SPACE)) {
		camera.updateCameraPos(CameraDirection::UP, dt);
	}

	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
		camera.updateCameraPos(CameraDirection::DOWN, dt);
	}

	double dx = Mouse::getDX(), dy = Mouse::getDY();
	if (dx != 0 || dy != 0) {
		camera.updateCameraDirection(dx, dy);
	}

	double scrollDY = Mouse::getScrollDY();
	if (scrollDY != 0) {
		camera.updateCameraZoom(scrollDY);
	}

	mainJ.update();
}
