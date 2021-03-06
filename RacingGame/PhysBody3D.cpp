#include "PhysBody3D.h"
#include "Globals.h"
#include "Application.h"
#include "Primitive.h"
#include "glmath.h"
#include "Bullet/include/btBulletDynamicsCommon.h"

// ---------------------------------------------------------
PhysBody3D::PhysBody3D()
	: body(nullptr)
	, colShape(nullptr)
	, motionState(nullptr)
	, parentPrimitive(nullptr)
	, collision_listeners()
{
	
}

PhysBody3D::PhysBody3D(btRigidBody* body) : body(body)
{
	body->setUserPointer(this);
}

// ---------------------------------------------------------
PhysBody3D::~PhysBody3D()
{
	if (HasBody() == true)
	{
		App->physics-> RemoveBodyFromWorld(body);
		delete body;
		delete colShape;
		delete motionState;
	}
}

void PhysBody3D::SetBody(Sphere* primitive, float mass)
{
	SetBody(new btSphereShape(primitive->GetRadius()),
		primitive, mass);
}

void PhysBody3D::SetBody(Cylinder* primitive, float mass) {

	SetBody(new btCylinderShape(btVector3(primitive->GetRadius(),primitive->GetRadius(),primitive->GetHeight() * 0.5)), primitive, mass);
}

void PhysBody3D::SetBody(Cube* primitive, float mass) {
	SetBody(new btBoxShape(btVector3(primitive->GetSize().x * 0.5,primitive->GetSize().y * 0.5,primitive->GetSize().z * 0.5)),
		primitive,mass);
}


bool PhysBody3D::HasBody() const
{
	return body != nullptr;
}

btRigidBody* PhysBody3D::GetBody() const
{
	return body;
}

// ---------------------------------------------------------
void PhysBody3D::GetTransform(float* matrix) const
{
	if (HasBody() == false)
		return;

	body->getWorldTransform().getOpenGLMatrix(matrix);
}

// ---------------------------------------------------------
void PhysBody3D::SetTransform(const float* matrix) const
{
	if (HasBody() == false)
		return;

	btTransform trans;
	trans.setFromOpenGLMatrix(matrix);
	body->setWorldTransform(trans);
	body->activate();
}

// ---------------------------------------------------------
void PhysBody3D::SetPos(float x, float y, float z)
{
	if (HasBody() == false)
		return;

	btTransform trans = body->getWorldTransform();
	trans.setOrigin(btVector3(x, y, z));
	body->setWorldTransform(trans);
	body->activate();
}

vec3 PhysBody3D::GetPos() const {
	vec3 position;
	mat4x4 transform;
	GetTransform(transform.M);
	position.x = transform.M[12];
	position.y = transform.M[13];
	position.z = transform.M[14];
	return position;
}

void PhysBody3D::SetSpeed(vec3 speed)
{
	Stop();
	Push(speed);
}

void PhysBody3D::Push(vec3 force)
{
	if (HasBody())
	{
		body->activate();
		body->applyCentralForce(btVector3(force.x, force.y, force.z));
	}
}

void PhysBody3D::Stop()
{
	if (HasBody())
		body->clearForces();
}

void PhysBody3D::SetBody(btCollisionShape * shape, Primitive* parent, float mass)
{
	assert(HasBody() == false);

	parentPrimitive = parent;

	colShape = shape;

	btTransform startTransform;
	startTransform.setFromOpenGLMatrix(&parent->transform);

	btVector3 localInertia(0, 0, 0);
	if (mass != 0.f)
		colShape->calculateLocalInertia(mass, localInertia);

	motionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, colShape, localInertia);

	body = new btRigidBody(rbInfo);

	body->setUserPointer(this);

	App->physics->AddBodyToWorld(body);
}

void PhysBody3D::SetAsSensor(bool is_sensor) {
	if (this->is_sensor != is_sensor)
	{
		this->is_sensor = is_sensor;
		if (is_sensor == true)
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		else
			body->setCollisionFlags(body->getCollisionFlags() &~ btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}
}

void PhysBody3D::SetLinearVelocity(float x, float y, float z) {
	btVector3 v(x, y, z);
	body->setLinearVelocity(v);
}

void PhysBody3D::SetAngularVelocity(float x, float y, float z)
{
	btVector3 v(x, y, z);
	body->setAngularVelocity(v);
}

void PhysBody3D::SetRotation(float x, float y, float z) {
	btTransform tr;
	tr.setIdentity();
	btQuaternion quat;
	quat.setEuler(y,x,z);
	tr.setRotation(quat);
	body->setCenterOfMassTransform(tr);
}