#include "Object3D.h"
#include "ShaderProgram.h"
#include <glm/ext.hpp>

glm::mat4 Object3D::buildModelMatrix() const {
	auto m = glm::translate(glm::mat4(1), m_position);
	m = glm::translate(m, m_center * m_scale);
	m = glm::rotate(m, m_orientation[2], glm::vec3(0, 0, 1));
	m = glm::rotate(m, m_orientation[0], glm::vec3(1, 0, 0));
	m = glm::rotate(m, m_orientation[1], glm::vec3(0, 1, 0));
	m = glm::scale(m, m_scale);
	m = glm::translate(m, -m_center);
	m = m * m_baseTransform;
	return m;
}

Object3D::Object3D(std::vector<Mesh3D>&& meshes)
	: Object3D(std::move(meshes), glm::mat4(1)) {
}

Object3D::Object3D(std::vector<Mesh3D>&& meshes, const glm::mat4& baseTransform)
	: m_meshes(meshes), m_position(), m_orientation(), m_scale(1.0),
	m_center(), m_baseTransform(baseTransform), m_material(0.1, 1.0, 0.3, 4)
{
}

const glm::vec3& Object3D::getPosition() const {
	return m_position;
}

const glm::vec3& Object3D::getOrientation() const {
	return m_orientation;
}

const glm::vec3& Object3D::getScale() const {
	return m_scale;
}

/**
 * @brief Gets the center of the object's rotation.
 */
const glm::vec3& Object3D::getCenter() const {
	return m_center;
}

const std::string& Object3D::getName() const {
	return m_name;
}

const glm::vec4& Object3D::getMaterial() const {
	return m_material;
}

size_t Object3D::numberOfChildren() const {
	return m_children.size();
}

const Object3D& Object3D::getChild(size_t index) const {
	return m_children[index];
}

Object3D& Object3D::getChild(size_t index) {
	return m_children[index];
}

void Object3D::setPosition(const glm::vec3& position) {
	m_position = position;
}

void Object3D::setOrientation(const glm::vec3& orientation) {
	m_orientation = orientation;
}

void Object3D::setScale(const glm::vec3& scale) {
	m_scale = scale;
}

/**
 * @brief Sets the center point of the object's rotation, which is otherwise a rotation around
   the origin in local space..
 */
void Object3D::setCenter(const glm::vec3& center)
{
	m_center = center;
}

void Object3D::setName(const std::string& name) {
	m_name = name;
}

void Object3D::setMaterial(const glm::vec4& material) {
	m_material = material;
}

void Object3D::move(const glm::vec3& offset) {
	m_position = m_position + offset;
}

void Object3D::rotate(const glm::vec3& rotation) {
	m_orientation = m_orientation + rotation;
}

void Object3D::grow(const glm::vec3& growth) {
	m_scale = m_scale * growth;
}

void Object3D::addChild(Object3D&& child) {
	m_children.emplace_back(child);
}

void Object3D::render(ShaderProgram& shaderProgram) const {
	renderRecursive(shaderProgram, glm::mat4(1));
}

/**
 * @brief Renders the object and its children, recursively.
 * @param parentMatrix the model matrix of this object's parent in the model hierarchy.
 */
void Object3D::renderRecursive(ShaderProgram& shaderProgram, const glm::mat4& parentMatrix) const {
	// This object's true model matrix is the combination of its parent's matrix and the object's matrix.
	glm::mat4 trueModel = parentMatrix * buildModelMatrix();
	shaderProgram.setUniform("model", trueModel);
	// Render each mesh in the object.
	for (auto& mesh : m_meshes) {
		mesh.render(shaderProgram);
	}
	// Render the children of the object.
	for (auto& child : m_children) {
		child.renderRecursive(shaderProgram, trueModel);
	}
}

//addedd physics functions defined
const glm::vec3& Object3D::getVelocity() const {
	return m_velocity;
}

void Object3D::setVelocity(const glm::vec3& vel) {
	m_velocity = vel;
}

const glm::vec3& Object3D::getRotVelocity() const {
	return m_rotVelocity;
}

void Object3D::setRotVelocity(const glm::vec3& rotVel) {
	m_rotVelocity = rotVel;
}

const glm::vec3& Object3D::getRotAcceleration() const {
	return m_rotAcceleration;
}

void Object3D::setRotAcceleration(const glm::vec3& rotAccel) {
	m_rotAcceleration = rotAccel;
}

float Object3D::getMass() const {
	return m_mass;
}

void Object3D::setMass(float mass) {
	m_mass = mass;
}

void Object3D::addConstantForce(const glm::vec3& force) {
	m_constantForces.push_back(force);
}

void Object3D::addAdditiveForce(const glm::vec3& direction, float magnitude) {
	AdditiveForce af;
	glm::vec3 dir = glm::normalize(direction);
	af.direction = dir;
	af.magnitude = magnitude;
	m_additiveForces.push_back(af);
}

void Object3D::tick(float dt, float dragCoefficient, float forceThreshold) {

	glm::vec3 totalForce(0.0f);
	for (auto& f : m_constantForces) {
		totalForce += f;
	}

	std::vector<AdditiveForce> remainingForces;
	remainingForces.reserve(m_additiveForces.size());

	for (auto& af : m_additiveForces) {
		totalForce += af.direction * af.magnitude;

		af.magnitude -= dragCoefficient * dt * af.magnitude;
		if (af.magnitude >= forceThreshold) {
			remainingForces.push_back(af);
		}
	}

	m_additiveForces = std::move(remainingForces);

	glm::vec3 acceleration = totalForce / m_mass;

	m_velocity += acceleration * dt;
	m_position += m_velocity * dt;

	m_rotVelocity += m_rotAcceleration * dt;
	m_orientation += m_rotVelocity * dt;
}
