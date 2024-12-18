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
#include <random>


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

//crreates an object
void createObject(Scene& scene, const std::string& path, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation = glm::vec3(0)) {
	auto object = assimpLoad(path, true);
	object.move(position);
	object.grow(scale);
	object.rotate(rotation);
	scene.objects.push_back(std::move(object));
}

//plays sound on a thread( lets you play multiple sounds)
void playSound(const std::string& filePath) {
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
			sf::sleep(sf::milliseconds(1000));
		}
		delete sound;
		delete buffer;
		}).detach();
}

// tilts pins up and down
float tiltPins(Scene& scene, float startDelay) {
	float tiltUpDuration = 1.3;
	float tiltDownDuration = 0.3f;
	float totalDelay = startDelay;

	for (int i = 2; i <= 11; ++i) {
		Animator animator;
		animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], totalDelay));
		animator.addAnimation(std::make_unique<RotationAnimation>(scene.objects[i], tiltUpDuration, glm::vec3(glm::radians(-40.0f), 0, 0)));
		animator.addAnimation(std::make_unique<RotationAnimation>(scene.objects[i], tiltDownDuration, glm::vec3(glm::radians(40.0f), 0, 0)));
		scene.animators.push_back(std::move(animator));
	}

	return totalDelay + tiltUpDuration + tiltDownDuration;
}

float cascadeJump(Scene& scene, float startDelay) {
	float jumpHeight = 0.5f;
	float jumpDuration = 0.5f;
	float delayBetweenRows = 0.2f;
	float totalDelay = startDelay;

	std::vector<std::vector<int>> rows = {
		{2}, 
		{3, 4}, 
		{5, 6, 7}, 
		{8, 9, 10, 11}
	};

	for (const auto& row : rows) {
		for (int pinIndex : row) {
			Animator animator;
			animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[pinIndex], totalDelay));
			animator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[pinIndex], jumpDuration / 2, glm::vec3(0, jumpHeight, 0)));
			animator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[pinIndex], jumpDuration / 2, glm::vec3(0, -jumpHeight, 0)));
			scene.animators.push_back(std::move(animator));
		}
		totalDelay += delayBetweenRows;
	}

	return totalDelay + jumpDuration;
}

float animatePins(Scene& scene, float startDelay) {
	float rotationDuration = 1.0f;
	float translationDuration = 2.0f;
	float delayDuration = 1.5f;
	float totalDelay = startDelay;

	for (int i = 2; i <= 11; ++i) {
		Animator animator;
		animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], totalDelay));
		animator.addAnimation(std::make_unique<RotationAnimation>(scene.objects[i], rotationDuration, glm::vec3(0, glm::radians(-180.0f), 0)));
		animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], delayDuration));
		animator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[i], translationDuration, glm::vec3(0, 0, -1.0f)));
		scene.animators.push_back(std::move(animator));
	}
	return totalDelay + rotationDuration + delayDuration + translationDuration;
}


float fireMissiles(Scene& scene, float startDelay) {
	glm::vec3 targetPosition = glm::vec3(0, -1, -6);
	glm::vec3 offScreenPosition = glm::vec3(0, -1000, 0);
	float moveDuration = 0.7f;
	float offScreenDuration = 0.00001f;
	float totalDelay = startDelay;

	for (int i = 12; i <= 16; ++i) {
		
		// orientate missiles to match angle of fire
		glm::vec3 currentPosition = scene.objects[i].getPosition();

		Animator missileAnimator;
		missileAnimator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], totalDelay));
		missileAnimator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[i], moveDuration, targetPosition - currentPosition));
		missileAnimator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[i], offScreenDuration, offScreenPosition - targetPosition));
		scene.animators.push_back(std::move(missileAnimator));
	}

	// make pins disapear either by animation or adding force to them.
	float pinDisappearDelay = totalDelay + moveDuration;
	std::thread([&scene, pinDisappearDelay]() {
		sf::sleep(sf::seconds(pinDisappearDelay));
			for (int i = 2; i <= 11; ++i) {
				scene.objects[i].resetForces();
				scene.objects[i].setMass(200.0f);
				float m = scene.objects[i].getMass();

				scene.objects[i].addConstantForce(glm::vec3(0.0f, -9.8f * m, 0.0f));
				if (i < 7)
				{
					scene.objects[i].addAdditiveForce(glm::vec3(1, 1, 0), 100.0f * m);
					scene.objects[i].setRotAcceleration(glm::vec3(-1, 2, 4));
				}
				else
				{
					scene.objects[i].addAdditiveForce(glm::vec3(-1, 1, 0), 100.0f * m);
					scene.objects[i].setRotAcceleration(glm::vec3(8, 0.5, -4));
				}

				//(dont need this if youre doing physics)
				/*Animator pinAnimator;
				pinAnimator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], totalDelay + moveDuration));
				pinAnimator.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[i], offScreenDuration, offScreenPosition - scene.objects[i].getPosition()));
				scene.animators.push_back(std::move(pinAnimator));*/
			}
		}).detach();

	
	return totalDelay + moveDuration + offScreenDuration;
}

// moves devil bowling ball in a bezier curve and makes it nod up and down
float moveDevil(Scene& scene, float startDelay) {
	float bezierDuration = 3.0f;
	float bounceDuration = 30.0f;
	float bounceFrequency = 20.0f;
	float totalDelay = startDelay;

	Animator bezierAnimator;
	bezierAnimator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[17], totalDelay));
	bezierAnimator.addAnimation(std::make_unique<BezierCurveAnimation>(scene.objects[17], bezierDuration, glm::vec3(1.5, 0.7, 3), glm::vec3(1.5, 0.7, 3), glm::vec3(0, 0.48, 3), glm::vec3(0, 0.41, 4.2)));
	scene.animators.push_back(std::move(bezierAnimator));

	Animator bouncingAnimator;
	bouncingAnimator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[17], totalDelay));
	bouncingAnimator.addAnimation(std::make_unique<SinBouncingAnimation>(scene.objects[17], bounceDuration, bounceFrequency));
	scene.animators.push_back(std::move(bouncingAnimator));
	
	/*Object3D& bowlingball = scene.objects[17].getChild(0);
	Animator bowlingballAnimator;
	bowlingballAnimator.addAnimation(std::make_unique<DelayAnimation>(bowlingball, totalDelay));
	bowlingballAnimator.addAnimation(std::make_unique<SinBouncingAnimation>(bowlingball, bounceDuration, bounceFrequency));
	scene.animators.push_back(std::move(bowlingballAnimator));*/

	/*Object3D& tail = scene.objects[17].getChild(1);
	Animator tailAnimator;
	tailAnimator.addAnimation(std::make_unique<DelayAnimation>(tail, totalDelay));
	tailAnimator.addAnimation(std::make_unique<SinBouncingAnimation>(tail, bounceDuration, bounceFrequency));
	scene.animators.push_back(std::move(tailAnimator));*/

	return totalDelay + bezierDuration;
}

float Hellstrike(Scene& scene, float startDelay = 0.0f) {
	float duration = 2.0f;

	Animator animator;
	animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[19], startDelay));
	animator.addAnimation(std::make_unique<MoveToAnimation>(scene.objects[19], duration, glm::vec3(0, 0.7, 4.1)));
	scene.animators.push_back(std::move(animator));

	return startDelay + duration;
}

void deletePin(Scene& scene, int pinIndex, const glm::vec3& shotDirection, std::set<int>& deletedPins) {
	if (deletedPins.find(pinIndex) != deletedPins.end())
	{
		return;
	}

	deletedPins.insert(pinIndex);
	scene.objects[pinIndex].resetForces();
	scene.objects[pinIndex].setMass(200.0f);
	float m = scene.objects[pinIndex].getMass();

	/*scene.objects[pinIndex].setVelocity(shotDirection * 100 * m);*/
	deletedPins.insert(pinIndex);
	scene.objects[pinIndex].addAdditiveForce(shotDirection, 400 * m);
	scene.objects[pinIndex].addAdditiveForce(glm::vec3(0, 1, 0), 200 * m);
	scene.objects[pinIndex].addConstantForce(glm::vec3(0.0f, -9.8f * m, 0.0f));

	//choose random death noise
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(1, 4);
	int randomSoundIndex = dist(gen);
	std::string soundFilePath = "C:/Users/ViniciusDugue/CECS 449/proj/sounds/death" + std::to_string(randomSoundIndex) + ".wav";
	playSound(soundFilePath);
}

bool isPinInAim(const Scene& scene, const glm::vec3& cameraPos, const glm::vec3& lookDirection, std::set<int>& deletedPins, int& hitPinIndex, float thresholdAngle = 1.0f) {
	for (int i = 2; i <= 11; ++i) {
		if (deletedPins.find(i) != deletedPins.end())
		{
			continue;
		}

		glm::vec3 pinPosition = scene.objects[i].getPosition();
		glm::vec3 toPin = glm::normalize(glm::vec3(pinPosition.x - cameraPos.x, 0, pinPosition.z - cameraPos.z));
		glm::vec3 flatLookDirection = glm::normalize(glm::vec3(lookDirection.x, 0, lookDirection.z));

		// angle between cameralook and pin.
		float angle = glm::degrees(glm::acos(glm::clamp(glm::dot(flatLookDirection, toPin), -1.0f, 1.0f)));
		if (angle <= thresholdAngle) {
			hitPinIndex = i;
			return true;
		}
	}

	hitPinIndex = -1;
	return false;
}

float resetPins(Scene& scene, const std::vector<glm::vec3>& originalPinPositions, float duration = 1.0f, float startDelay = 1.0f) {
	float totalDelay = startDelay;

	for (int i = 2; i <= 11; ++i) {
		scene.objects[i].resetForces();
		glm::vec3 originalPosition = originalPinPositions[i-2];

		Animator animator;
		animator.addAnimation(std::make_unique<DelayAnimation>(scene.objects[i], totalDelay));
		animator.addAnimation(std::make_unique<MoveToAnimation>(scene.objects[i], duration, originalPosition));
		scene.animators.push_back(std::move(animator));
		std::cout << "pin: "<<i<<"Position:"<< scene.objects[i].getPosition() << std::endl;
	}

	return totalDelay + duration;
}


/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
//Scene bunny() {
//	Scene scene{ texturingShader() };
//
//	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
//	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
//	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
//	auto bunny = assimpLoad("models/bunny_textured.obj", true);
//	bunny.grow(glm::vec3(9, 9, 9));
//	bunny.move(glm::vec3(0.2, -1, 0));
//	
//	// Move all objects into the scene's objects list.
//	scene.objects.push_back(std::move(bunny));
//	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
//	// scene.objects[0]
//
//	Animator spinBunny;
//	// Spin the bunny 360 degrees over 10 seconds.
//	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
//	
//	// Move all animators into the scene's animators list.
//	scene.animators.push_back(std::move(spinBunny));
//
//	return scene;
//}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
// Scene marbleSquare() {
// 	Scene scene{ phongLightingShader() };// texturingShader()

// 	std::vector<Texture> textures = {
// 		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
// 	};
// 	auto mesh = Mesh3D::square(textures);
// 	auto floor = Object3D(std::vector<Mesh3D>{mesh});
// 	floor.grow(glm::vec3(5, 5, 5));
// 	floor.move(glm::vec3(0, -1.5, 0));
// 	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));

// 	scene.objects.push_back(std::move(floor));
// 	return scene;
// }

/**
 * @brief Loads a cube with a cube map texture.
 */
//Scene cube() {
//	Scene scene{ phongLightingShader() };//texturingShader()
//
//	auto cube = assimpLoad("models/cube.obj", true);
//
//	scene.objects.push_back(std::move(cube));
//
//	Animator spinCube;
//	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
//	// Then spin around the x axis.
//	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(2 * M_PI, 0, 0)));
//
//	scene.animators.push_back(std::move(spinCube));
//
//	scene.program.activate();
//	scene.program.setUniform("color", glm::vec3(1.0f, 0.0f, 0.0f));
//
//	return scene;
//}

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
	Scene scene{ phongLightingShader() };

	createObject(scene, "models/bowlingalley/alley.obj", glm::vec3(0, -0.8, 0.5), glm::vec3(0.2, 0.2, 0.5));

	createObject(scene, "models/Skybox/hellskybox.obj", glm::vec3(0, 0, -40), glm::vec3(20, 20, 20), glm::vec3(glm::radians(90.0f), glm::radians(-90.0f), 0));
	
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.0, -0.8, -2.8), glm::vec3(0.2, 0.2, 0.2));

	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(-0.3, -0.8, -3.6), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.3, -0.8, -3.6), glm::vec3(0.2, 0.2, 0.2));

	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(-0.6, -0.8, -4.2), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.0, -0.8, -4.2), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.6, -0.8, -4.2), glm::vec3(0.2, 0.2, 0.2));

	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(-0.9, -0.8, -5), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(-0.3, -0.8, -5), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.3, -0.8, -5), glm::vec3(0.2, 0.2, 0.2));
	createObject(scene, "models/bowlingpin/Pin.obj", glm::vec3(0.9, -0.8, -5), glm::vec3(0.2, 0.2, 0.2));

	createObject(scene, "models/missile/missile1.obj", glm::vec3(-6, 7, 3), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.637f,0.588f,0));
	createObject(scene, "models/missile/missile1.obj", glm::vec3(-3, 7, 3), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.701, 0.322, 0));
	createObject(scene, "models/missile/missile1.obj", glm::vec3(0, 7, 3), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.727, 0.000, 0));
	createObject(scene, "models/missile/missile1.obj", glm::vec3(3, 7, 3), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.701, -0.322, 0));
	createObject(scene, "models/missile/missile1.obj", glm::vec3(6, 7, 3), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.637, -0.588, 0));

	createObject(scene, "models/bowlingdevil/devilbowlingball_tail.obj", glm::vec3(20, -0.5, 0), glm::vec3(0.2, 0.2, 0.2), glm::vec3(0, glm::radians(90.0f), 0));

	createObject(scene, "models/doomui/doomui.obj", glm::vec3(0, 0.4935, 4.85), glm::vec3(0.065, 0.065, 0.065), glm::vec3(0, 0, 0));
	createObject(scene, "models/StrikeVisual/hellstrike.obj", glm::vec3(0, -2, 4.1), glm::vec3(0.5, 0.5, 0.05), glm::vec3(0, 0, 0));

	return scene;
}

int main() {
	
	std::cout << std::filesystem::current_path() << std::endl;

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
	auto myScene = bowlingBall();// ;// cube();// lifeOfPi();
	// You can directly access specific objects in the scene using references.
	auto& firstObject = myScene.objects[0];

	// Activate the shader program.
	myScene.program.activate();

	// Set up the view and projection matrices.

	//light space matrix setup
	glm::vec3 lightPos = glm::vec3(-0.6, 3, -4.2);//.-0.6, 3, -4.2
	glm::vec3 lightTarget = glm::vec3(-0.6, -0.8, -4.2);
	glm::mat4 lightProjection = glm::ortho(-10.0, 10.0, -10.0, 10.0, 0.5, 4.5);
	glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0,0,1));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;

	glm::vec3 lightDirection = lightPos - lightTarget;

	myScene.program.setUniform("lightSpaceMatrix", lightSpaceMatrix);
	myScene.program.setUniform("directionalLight", lightDirection);//glm::vec3(2, 2, 2)
	myScene.program.setUniform("directionalColor", glm::vec3(0.5f, 0.5f, 1.0f));
	myScene.program.setUniform("ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	myScene.program.setUniform("material", glm::vec4(0.8f, 0.7f, 0.8f, 30.0f));

	// initializing shadow/depth map frame buffers and shadow shaders(these are just for depth calculation)
	unsigned int shadowMap;
	unsigned int shadowFBO;

	//make framebuffe and shadow texture
	glGenFramebuffers(1, &shadowFBO);
	glGenTextures(1, &shadowMap);

	//sets shadow map texture as the active one
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 0.5, 0.5, 0.5, 0.5 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

	// disable color so we only get depth
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	//switch back to og buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping

	//initialize shadow shaders and set some unifoms
	ShaderProgram shadowShader;
	shadowShader.load("shaders/shadow.vert", "shaders/shadow.frag");
	shadowShader.activate();
	shadowShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

	//initialize camera moving variables
	float cameraYaw = 0.0f; 
	const float maxYaw = 60.0f;
	glm::vec3 cameraPos = glm::vec3(0, 1, 5); 
	glm::vec3 lookAt = glm::vec3(0, 0, 0); 
	glm::vec3 cameraUp = glm::vec3(0, 1, 0);
	bool firstMouse = true;
	float lastMouseX = 0.0f;

	// initialize game variables
	int pinCounter = 0;
	bool isStrikePlayed = false;
	glm::vec3 pinDeletionTranslation = glm::vec3(0, -5, 10);
	std::set<int> deletedPins;
	std::vector<glm::vec3> originalPinPositions = {
	glm::vec3(0.0, -0.8, -2.8), 
	glm::vec3(-0.3, -0.8, -3.6), glm::vec3(0.3, -0.8, -3.6),
	glm::vec3(-0.6, -0.8, -4.2), glm::vec3(0.0, -0.8, -4.2), glm::vec3(0.6, -0.8, -4.2),
	glm::vec3(-0.9, -0.8, -5.0), glm::vec3(-0.3, -0.8, -5.0), glm::vec3(0.3, -0.8, -5.0), glm::vec3(0.9, -0.8, -5.0)};

	playSound("C:/Users/ViniciusDugue/CECS 449/proj/sounds/doomost.wav");

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
				if (ev.key.code == sf::Keyboard::W) {
					playSound("C:/Users/ViniciusDugue/CECS 449/proj/sounds/discord.mp3");
				}
			}

			if (ev.type == sf::Event::MouseButtonPressed && isStrikePlayed ==false) {
				if (ev.mouseButton.button == sf::Mouse::Left) {
					glm::vec3 rayDirection = glm::normalize(lookAt - cameraPos);
					bool pinHit = false;
					
					float aimThreshold = 1.0f;
					int hitPinIndex = -1;
					if (isPinInAim(myScene, cameraPos, rayDirection, deletedPins, hitPinIndex)) {

						glm::vec3 flatLookDirection = glm::normalize(glm::vec3(rayDirection.x, 0, rayDirection.z));

						//deletePin(myScene, hitPinIndex, pinDeletionTranslation, deletedPins, 0.1f);
						deletePin(myScene, hitPinIndex, flatLookDirection, deletedPins);
						std::cout << "Pin hit: " << hitPinIndex << " " << std::endl;
						pinCounter += 1;
					}
					
					// calling this to run animators in this if block
					for (auto& anim : myScene.animators) {
						anim.start();
					}
				}
			}
			if (pinCounter == 10 && isStrikePlayed == false)
			{
				std::cout << "10 mins hit, reseting pins..." << std::endl;

				pinCounter = 0;
				isStrikePlayed = true;

				//add delay for deleting pins with physics(last pin will disappear instead of seeing it fly away wcyd
				for (auto& o : myScene.objects) {
					float dragCoefficient = 10.0f;
					float forceThreshold = 1.0f;
					o.tick(0.3, dragCoefficient, forceThreshold);

					glm::vec3 pos = o.getPosition();
					glm::vec3 vel = o.getVelocity();

					o.setPosition(pos);
					o.setVelocity(vel);
				}

				float resetPinDuration = resetPins(myScene, originalPinPositions, 1.5f, 2.0f);

				float tiltDuration = tiltPins(myScene, resetPinDuration);

				float jumpDuration = cascadeJump(myScene, tiltDuration);

				float pinRunDuration = animatePins(myScene, jumpDuration);

				float fireMissileDuration = fireMissiles(myScene, pinRunDuration - 1.2);

				float moveDevilDuration  = moveDevil(myScene, fireMissileDuration + 1.0f);

				Hellstrike(myScene, moveDevilDuration);

				// play some sounds before animations finish woth threads
				std::thread([moveDevilDuration]() {
					sf::sleep(sf::seconds(moveDevilDuration-2));
					playSound("C:/Users/ViniciusDugue/CECS 449/proj/sounds/bowserlaugh.wav");
					}).detach();

				std::thread([moveDevilDuration]() {
					sf::sleep(sf::seconds(moveDevilDuration ));
					playSound("C:/Users/ViniciusDugue/CECS 449/proj/sounds/wiistrike.wav");
					}).detach();

				std::thread([moveDevilDuration]() {
					sf::sleep(sf::seconds(moveDevilDuration +1.5));
					playSound("C:/Users/ViniciusDugue/CECS 449/proj/sounds/hellstrike.wav");
					}).detach();

				// calling animators for this if block
				for (auto& anim : myScene.animators) {
					anim.start();
				}
			}

			if (ev.type == sf::Event::MouseMoved)
			{
				if (firstMouse) {
					lastMouseX = ev.mouseMove.x;
					firstMouse = false;
				}

				float deltaX = ev.mouseMove.x - lastMouseX;
				lastMouseX = ev.mouseMove.x;

				float sensitivity = 0.1f;
				float rotationAmount = deltaX * sensitivity;
				cameraYaw += rotationAmount; 

				if (cameraYaw > maxYaw) cameraYaw = maxYaw;
				if (cameraYaw < -maxYaw) cameraYaw = -maxYaw;

				float yawRadians = glm::radians(cameraYaw);
				lookAt.x = sin(yawRadians) * (cameraPos.z - 0);
				lookAt.z = -cos(yawRadians) * (cameraPos.z - 0);
				lookAt.y = 0.0f;


				//moving doom ui object so its in front of camera and changing orientation to match camera
				float uiDistance = 0.15f;
				glm::vec3 forwardDirection = glm::normalize(lookAt - cameraPos);
				forwardDirection.y = 0.0f;

				glm::vec3 uiPosition = cameraPos + forwardDirection * uiDistance;
				uiPosition.y = 0.4925;
				myScene.objects[18].setPosition(uiPosition);


				float yaw = std::atan2(-forwardDirection.x, -forwardDirection.z);
				glm::vec3 uiOrientation = glm::vec3(0.0f, yaw, 0.0f);

				myScene.objects[18].setOrientation(uiOrientation);

			}
		}
		
		auto now = c.getElapsedTime();
		auto diff = now - last;
		/*std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;*/
		last = now;

		float dt = diff.asSeconds();

		for (auto& o : myScene.objects) {
			float dragCoefficient = 10.0f;
			float forceThreshold = 1.0f;
			o.tick(dt, dragCoefficient, forceThreshold);

			glm::vec3 pos = o.getPosition();
			glm::vec3 vel = o.getVelocity();

			o.setPosition(pos);
			o.setVelocity(vel);
		}

		glm::vec3 cameraPos = glm::vec3(0, 0.5, 5);
		glm::mat4 perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.01, 100.0);
		glm::mat4 camera = glm::lookAt(cameraPos, lookAt, cameraUp);

		myScene.program.setUniform("view", camera);
		myScene.program.setUniform("projection", perspective);
		myScene.program.setUniform("cameraPos", cameraPos);

		// Update the scene.
		for (auto& anim : myScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// change vertiport to shadowmap res and swap frame to shadow frame buffer
		glViewport(0, 0, 1024, 1024);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		//first shadow map pass to render depth/shadow map
		shadowShader.activate();
		shadowShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
		for (auto& o : myScene.objects) {

			o.render(shadowShader);
		}

		// change back to og buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// change back the vertiport
		glViewport(0, 0, window.getSize().x, window.getSize().y);

		myScene.program.activate();

		//put shadow map in texture 0
		myScene.program.setUniform("shadowMap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMap);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Second pass to finally render the scene objects
		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}

		window.display();
	}

	return 0;
}


