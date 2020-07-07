#include "Random.h"
#include <ctime>
#include <math.h>

int Game::Random::seed = -1;

float Game::Random::rand() {
	if (seed < 0) {
		seed = clock();
	}

	float result = sin(static_cast<float>(seed++)) * 114519.14;
	result = result - floor(result);
	return result;
}

Game::Vector2 Game::Random::rand2() {
	return Game::Vector2(rand(),rand());
}

Game::Vector3 Game::Random::rand3() {
	return Game::Vector3(rand(),rand(),rand());
}

Game::Vector4 Game::Random::rand4() {
	return Game::Vector4(rand(),rand(),rand(),rand());
}