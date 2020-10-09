#pragma once
#include "IApplication.h"
#include "GraphicModule.h"
#include "Config.h"
#include <Windows.h>
#include <vector>

namespace Game {
	class WindowsApplication : public IApplication{
		friend class EditorGUIModule;
	public:
		virtual bool initialize() override;
		virtual void tick() override;
		virtual bool isQuit() override;
		virtual void finalize() override;
		virtual void messageBox(const char* title, const char* content) override{
			MessageBoxA(NULL,content,title,MB_OK);
		}
		virtual void Quit() override;

		virtual bool YesNoBox(const char* title,const char* content) override {
			return MessageBoxA(NULL,content,title,MB_YESNO) == IDYES;
		}

		virtual std::string getPath(const char* filter) override;

		virtual void setTitle(const char* title) override;

		WindowsApplication(int width, int height,IRuntimeModule** runtimeModules,int moduleNum) :
		config(width,height),
		moduleList(moduleNum),winHandle(NULL),
		hinstance(NULL){
			for (int i = 0; i != moduleNum; i++) {
				moduleList[i] = runtimeModules[i];
			}
		}

		HWND getMainWnd() { return winHandle; }

		virtual const Config& getSysConfig() override { return config; }

		virtual void resize(uint32_t width,uint32_t height) override;
	private:
		bool quit = false;
		HWND winHandle;
		HINSTANCE hinstance;
		
		//GraphicModule* backEnd;
		std::vector<IRuntimeModule*> moduleList;
		Config config;
	};


}