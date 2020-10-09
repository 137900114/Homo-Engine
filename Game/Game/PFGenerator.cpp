#include "PFGenerator.h"

using namespace Game;

void PFRegister::tick(float deltatime) {
	for (int i = 0; i != pForces.size(); i++) {
		pForces[i].Force->UpdateForce(pForces[i].PhyPartical, deltatime);
	}
}

void PFRegister::Register(PhyPartical* partical, PFGenerator* force) {
	pForces.push_back(PFRegistration{partical, force});
}

bool PFRegister::UnRegister(PhyPartical* partical,PFGenerator* force) {
	for (auto iter = pForces.begin(); iter != pForces.end(); iter++) {
		if (iter->Force == force && iter->PhyPartical == partical) {
			pForces.erase(iter);
			return true;
		}
	}
	return false;
}

PFGravity PFGravity::DefaultGravity = PFGravity();

void PFGravity::UpdateForce(PhyPartical* partical,float /*deltatime*/) {
	if (partical->isMassInfinitive())
		return;
	partical->AddForce(Vector3(0, -gravityScale, 0) * partical->GetMass());
}

void PFFriction::UpdateForce(PhyPartical* partical,float /*deltatime*/) {
	Vector3 vocel = partical->GetVocel();
	float vLen = length(vocel);
	Vector3 vDir = normalize(vLen);

	Vector3 force = vDir * vLen * (k1 + vLen * k2);

	partical->AddForce(force);
}

void PFSpring::UpdateForce(PhyPartical* partical, float /*deltatime*/) {

	Vector3 d = partical->GetPosition() - other->GetPosition();
	float disLen = length(d);
	Vector3 dir = Vector3() - normalize(d);

	Vector3 force = dir * (disLen - originLength) * springConstant;
	partical->AddForce(force);
}

void PFAnchoredSpring::UpdateForce(PhyPartical* partical,float /*deltatime*/) {
	Vector3 d = partical->GetPosition() - anchor;
	float disLen = length(d);
	Vector3 dir = Vector3() - normalize(d);

	Vector3 force = dir * (disLen - originLength) * springConstant;
	partical->AddForce(force);
}


void PFHardAnchoredSpring::UpdateForce(PhyPartical* partical,float deltatime) {
	if (gamma < 0) return;

	Vector3 p0 = partical->GetPosition();
	Vector3 v0 = partical->GetVocel();

	Vector3 c = p0 * (damping * .5f * invGamma) + v0 * invGamma;

	Vector3 pt = (p0 * cos(gamma * deltatime) + c * sin(gamma * deltatime)) * exp(- damping * .5f * deltatime);
	Vector3 acceler = (p0 - pt) * (1.f / (deltatime * deltatime)) - v0 * (1.f / deltatime);

	partical->AddForce(acceler * partical->GetMass());
}