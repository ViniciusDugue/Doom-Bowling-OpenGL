/**
This application displays a mesh in wireframe using "Modern" OpenGL 3.0+.
The Mesh3D class now initializes a "vertex array" on the GPU to store the vertices
	and faces of the mesh. To render, the Mesh3D object simply triggers the GPU to draw
	the stored mesh data.
We now transform local space vertices to clip space using uniform matrices in the vertex shader.
	See "simple_perspective.vert" for a vertex shader that uses uniform model, view, and projection
		matrices to transform to clip space.
	See "uniform_color.frag" for a fragment shader that sets a pixel to a uniform parameter.
*/
#define _USE_MATH_DEFINES
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <math.h>

#define GLM_ENABLE_EXPERIMENTAL

#include "AssimpImport.h"
#include "Mesh3D.h"
#include "Object3D.h"
#include "Animator.h"
#include "ShaderProgram.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <glm/gtx/string_cast.hpp>

#include <fstream>

#include <SFML/Audio.hpp>
#include <thread>

struct Scene {
	ShaderProgram program;
	std::vector<Object3D> objects;
	std::vector<Animator> animators;
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader;
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader;
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;

}
ShaderProgram createUniformColorShader() {
	ShaderProgram shader;
	shader.load("shaders/uniform_color.vert", "shaders/uniform_color.frag");
	return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i;
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}
glm::vec3 calculateBoundingBox(const std::string& objFilePath) {
	std::ifstream file(objFilePath);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open OBJ file: " + objFilePath);
	}

	glm::vec3 minPoint(std::numeric_limits<float>::max());
	glm::vec3 maxPoint(std::numeric_limits<float>::lowest());

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			minPoint = glm::min(minPoint, glm::vec3(x, y, z));
			maxPoint = glm::max(maxPoint, glm::vec3(x, y, z));
		}
	}

	file.close();
	return maxPoint - minPoint; 
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ texturingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny = assimpLoad("models/bunny_textured.obj", true);
	bunny.grow(glm::vec3(9, 9, 9));
	bunny.move(glm::vec3(0.2, -1, 0));
	
	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
	// scene.objects[0]

	Animator spinBunny;
	// Spin the bunny 360 degrees over 10 seconds.
	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	
	// Move all animators into the scene's animators list.
	scene.animators.push_back(std::move(spinBunny));

	return scene;
}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ phongLightingShader() };// texturingShader()

	std::vector<Texture> textures = {
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh = Mesh3D::square(textures);
	auto floor = Object3D(std::vector<Mesh3D>{mesh});
	floor.grow(glm::vec3(5, 5, 5));
	floor.move(glm::vec3(0, -1.5, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ createUniformColorShader() };//texturingShader()

	auto cube = assimpLoad("models/cube.obj", true);

	scene.objects.push_back(std::move(cube));

	Animator spinCube;
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	// Then spin around the x axis.
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(2 * M_PI, 0, 0)));

	scene.animators.push_back(std::move(spinCube));

	scene.program.activate();
	scene.program.setUniform("color", glm::vec3(1.0f, 0.0f, 0.0f));

	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ phongLightingShader() };

	auto boat = assimpLoad("models/boat/boat.fbx", true);
	boat.move(glm::vec3(0, -0.7, 0));
	boat.grow(glm::vec3(0.01, 0.01, 0.01));
	auto tiger = assimpLoad("models/tiger/scene.gltf", true);
	tiger.move(glm::vec3(0, -5, 10));
	// Move the tiger to be a child of the boat.
	boat.addChild(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));

	// We want these animations to referenced the *moved* objects, which are no longer
	// in the variables named "tiger" and "boat". "boat" is now in the "objects" list at
	// index 0, and "tiger" is the index-1 child of the boat.
	Animator animBoat;
	animBoat.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10, glm::vec3(0, 2 * M_PI, 0)));
	Animator animTiger;
	animTiger.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0].getChild(1), 10, glm::vec3(0, 0, 2 * M_PI)));

	// The Animators will be destroyed when leaving this function, so we move them into
	// a list to be returned.
	scene.animators.push_back(std::move(animBoat));
	scene.animators.push_back(std::move(animTiger));

	// Transfer ownership of the objects and animators back to the main.
	return scene;
}

Scene bowlingBall() {
	Scene scene{ phongLightingShader() }; //createUniformColorShader ()

	auto alley = assimpLoad("models/bowlingalley/alley.obj", true);//models/bowlingdevil/devilbowlingball.obj models/Tree/tree01.obj models/bowlingpin/Pin.obj
	auto devilbowlingball = assimpLoad("models/bowlingalley/alley.obj", true)

	ball.move(glm::vec3(0, -0.5, 0));
	ball.grow(glm::vec3(0.2f,0.2f, 0.2f));
	 

	auto boat = assimpLoad("models/boat/boat.fbx", true);
	boat.move(glm::vec3(0, -0.7, 0));
	boat.grow(glm::vec3(0.01, 0.01, 0.01));

	auto skybox = assimpLoad("models/Skybox/hellskybox.obj", true);//models/bowlingdevil/devilbowlingball.obj models/Tree/tree01.obj models/bowlingpin/Pin.obj

	skybox.move(glm::vec3(0, 0, -40));
	skybox.grow(glm::vec3(20, 20, 20));
	skybox.rotate(glm::vec3(glm::radians(90.0f), glm::radians(-90.0f),0 ));
	scene.objects.push_back(std::move(ball));
	scene.objects.push_back(std::move(boat));
	scene.objects.push_back(std::move(skybox));

	Animator ballAnimator;

	ballAnimator.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10, glm::vec3(0, 0.5 * M_PI, 0)));

	scene.animators.push_back(std::move(ballAnimator));

	return scene;
}
void playSoundAsync(const std::string& filePath) {
	auto* buffer = new sf::SoundBuffer();
	if (!buffer->loadFromFile(filePath)) {
		delete buffer;      
		return;
	}

	auto* sound = new sf::Sound();
	sound->setBuffer(*buffer);
	sound->play();

	std::thread([sound, buffer]() {
		while (sound->getStatus() == sf::Sound::Playing) {
			sf::sleep(sf::milliseconds(100)); 
		}
		delete sound;
		delete buffer;
		}).detach(); 
}

int main() {
	
	std::cout << std::filesystem::current_path() << std::endl;

	glm::vec3 boundingBoxSize = calculateBoundingBox("models/bowlingpin/Pin.obj");///"models/bowlingdevil/BOWLING BALL.obj" models/bowlingpin/Bowling_Pin.obj
	std::cout << "Bounding box size (width, height, depth): "
		<< boundingBoxSize.x << ", "
		<< boundingBoxSize.y << ", "
		<< boundingBoxSize.z << std::endl;
	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	settings.majorVersion = 3;
	settings.minorVersion = 3;
	sf::Window window(sf::VideoMode{ 1200, 800 }, "Modern OpenGL", sf::Style::Resize | sf::Style::Close, settings);

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);

	// Inintialize scene objects.
	auto myScene = bowlingBall(); ; // ;// cube();// lifeOfPi();
	// You can directly access specific objects in the scene using references.
	auto& firstObject = myScene.objects[0];

	// Activate the shader program.
	myScene.program.activate();

	// Set up the view and projection matrices.

	myScene.program.setUniform("directionalLight", glm::vec3(2.0f, 2.0f, 2.0f));
	myScene.program.setUniform("directionalColor", glm::vec3(0.5f, 0.5f, 1.0f));
	myScene.program.setUniform("ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	myScene.program.setUniform("material", glm::vec4(0.8f, 0.7f, 0.8f, 30.0f));

	/*playSoundAsync("C:/Users/ViniciusDugue/CECS 449/proj/sounds/wiistrike.wav");

	playSoundAsync("C:/Users/ViniciusDugue/CECS 449/proj/sounds/bowserlaugh.wav");

	playSoundAsync("C:/Users/ViniciusDugue/CECS 449/proj/sounds/doomost.wav");*/

	// Ready, set, go!
	bool running = true;
	sf::Clock c;
	auto last = c.getElapsedTime();

	// Start the animators.
	for (auto& anim : myScene.animators) {
		anim.start();
	}

	while (running) {
		
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) {
				running = false;
			}

			if (ev.type == sf::Event::KeyPressed) {
				if (ev.key.code == sf::Keyboard::Space) {
					playSoundAsync("C:/Users/ViniciusDugue/CECS 449/proj/sounds/bowserlaugh.wav");
				}
			}
		}
		auto now = c.getElapsedTime();
		auto diff = now - last;
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
		last = now;


		glm::vec3 cameraPos = glm::vec3(0, 0, 5);
		glm::mat4 camera = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0);
		myScene.program.setUniform("view", camera);
		myScene.program.setUniform("projection", perspective);
		myScene.program.setUniform("cameraPos", cameraPos);


		// Update the scene.
		for (auto& anim : myScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Render the scene objects.
		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}
		window.display();


		
	}

	return 0;
}


