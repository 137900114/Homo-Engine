#include "DebugLayer.h"
#include "GraphicModule.h"
#include "Common.h"
#include "GeometryGenerator.h"

namespace Game {
	extern GraphicModule* gGraphic;
}


using namespace Game;

void Debug::log(const char* str) {
	Log(str);
}

template<typename ...T>
void Debug::log(const char* format,T ...args) {
	Log(format, args...);
}

bool Debug::initialize() {
	this->cubeMesh = std::move(GeometryGenerator::generateCube());
	this->sphereMesh = std::move(GeometryGenerator::generateSphere(true));
	this->planeMesh = std::move(GeometryGenerator::generatePlane(10,10,Y_AXIS));
	
	return true;
}

void Debug::finalize() {

}

void Debug::tick() {

}