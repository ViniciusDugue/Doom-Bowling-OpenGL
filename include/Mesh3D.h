#pragma once
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <vector>

#include "Texture.h"
#include "ShaderProgram.h"
struct Vertex3D {
	float x;
	float y;
	float z;

	float nx;
	float ny;
	float nz;

	float u;
	float v;

	//wtf, it turns out if you define these variables before the uv variables above, then the texture mapping 
	// gets all messed up. Why is this the case????
	float tx = 0.0f, ty = 0.0f, tz = 0.0f;
	float bx = 0.0f, by = 0.0f, bz = 0.0f;

	Vertex3D(float px, float py, float pz, float normX, float normY, float normZ,
		float texU, float texV, float tanX, float tanY, float tanZ, float bitX, float bitY, float bitZ) :
		x(px), y(py), z(pz), nx(normX), ny(normY), nz(normZ), u(texU), v(texV),
		tx(tanX), ty(tanY), tz(tanZ), bx(bitX), by(bitY), bz(bitZ) {}
};

class Mesh3D {
private:
	uint32_t m_vao;
	std::vector<Texture> m_textures;
	uint32_t m_vertexCount;
	uint32_t m_faceCount;

public:
	Mesh3D() = delete;

	
	/**
	 * @brief Construcst a Mesh3D using existing vectors of vertices and faces.
	*/
	Mesh3D(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& faces);

	Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces,
		Texture texture);

	Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces,
		std::vector<Texture>&& textures);

	void addTexture(Texture texture);

	/**
	 * @brief Constructs a 1x1 square centered at the origin in world space.
	*/
	/*static Mesh3D square(const std::vector<Texture>& textures);*/
	
	/**
	 * @brief Renders the mesh to the given context.
	 * @param model the local->world model transformation matrix.
	 * @param view the world->view camera matrix.
	 * @param proj the view->clip projection matrix.
	*/
	void render(ShaderProgram& program) const;
	
};
