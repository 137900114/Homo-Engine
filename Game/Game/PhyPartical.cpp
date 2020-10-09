#include "PhyPartical.h"


using namespace Game;

void PhyPartical::Intergrate(float deltatime) {

	this->Vocel += this->Acceleration * deltatime + this->Impact * this->invMass;
	this->Position += this->Vocel * deltatime;

	this->Acceleration = Vector3();
	this->Impact = Vector3();
}