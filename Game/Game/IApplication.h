#pragma once

#include "IRuntimeModule.hpp"
#include "Config.h"
#include "Common.h"
#include <string>

namespace Game {
	class IApplication : public IRuntimeModule {
	public:
		virtual bool initialize() = 0;
		virtual void tick() = 0;
		virtual void finalize() = 0;

		virtual bool isQuit() = 0;

		virtual void messageBox(const char* title, const char* content) { Log("message box is not supported in this system\n"); }
		virtual bool YesNoBox(const char* title, const char* content) { Log("yes and no box is not supportted on this system\n"); return false; }
		virtual void setTitle(const char* title) { Log("changing window title is not supported in this system\n"); }

		virtual std::string getPath(const char* filter = nullptr) { Log("this plantform doesn't support getting path\n"); return std::string(); }

		virtual ~IApplication() {}

		virtual const Config& getSysConfig() = 0;

		virtual void Quit() = 0;

		virtual void resize(uint32_t width,uint32_t height) = 0;
	};
}