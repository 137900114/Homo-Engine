#pragma once

namespace Game {
	struct Config {
		int width, height,top,left;
		Config(float width, float height,int top = 0.,int left = 0.) :
			width(width), height(height),top(top),left(left) {}
	};
}