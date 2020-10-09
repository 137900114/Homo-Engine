#include "PhyRigidBody.h"

using namespace Game;

void PhyRigidBody::intergrate(float deltatime) {

	Mat3x3 worldInvInertia = GetWorldInvInertia(transform,invInertia);

	angleSpeed += mul(worldInvInertia, torqueAccum * deltatime + angleImpulse);
	velocity += (forceAccum * deltatime + Impulse) * invMass;

	position += velocity * deltatime;

	rotation = rotation + Game::transform(angleSpeed,rotation) * deltatime * .5f;

	ClearForceAccum();

	transform = mul(MatrixPosition(position),rotation.Mat());

}