#pragma once
#include "IRuntimeModule.hpp"
#include "GraphicModule.h"
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <stdint.h>
#include <unordered_map>
#include "PipelineStateObject.h"
#include "FileLoader.h"
#include <memory>
#include "Matrix.h"
#include "d3dCommon.h"
#include "ShaderLoader.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
//we assume that if we use direct3d as render module 
//the concurrent plantfrom the program runnning on must be windows

namespace Game {
	class D3DRenderModule : public GraphicModule {

		using InputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;
	public:

		virtual bool initialize() override;
		virtual void tick() override;
		virtual void finalize() override;

		virtual void point2D(Vector2 pos,float size,Vector4 Color,float depth) override;
		virtual void line2D(Vector2 start,Vector2 end,float width,Vector4 Color,float depth) override;
		virtual void cricle2D(Vector2 pos, float radius, Vector4 Color, float depth, int powPoly) override;


		virtual void set2DViewPort(Vector2 center,float height) override;

		virtual void bindScene(Scene* scene) override;
		virtual void releaseScene(Scene& scene) override;
		virtual void uploadScene(Scene& scene) override;

		virtual void drawSingleMesh(Mesh& mesh,SceneMaterial* mat) override;
	private:

		bool createRenderTargetView();
		void waitForFenceValue(uint64_t fence);

		bool initialize2D();
		bool initializeDefault();
		void draw2D();

		int width;
		int height;

		HWND winMain;
		ComPtr<IDXGIFactory4> mDxgiFactory;
		ComPtr<IDXGISwapChain3> mSwapChain;
		ComPtr<ID3D12Device> mDevice;

		static const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static const DXGI_FORMAT mDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		static const int mBackBufferNum = 3;


		ComPtr<ID3D12Resource> mBackBuffers[mBackBufferNum];

		//------------------//
		ComPtr<ID3D12Resource> mDepthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		//------------------//

		ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		int rtvDescriptorIncreament;
		
		ComPtr<ID3D12CommandQueue> mCmdQueue;

		//-----------------//
		ComPtr<ID3D12CommandAllocator> mCmdAllc;
		ComPtr<ID3D12GraphicsCommandList> mCmdList;
		//----------------//

		D3D12_VIEWPORT viewPort;
		D3D12_RECT sissorRect;

		int mCurrentFrameIndex;
		ComPtr<ID3D12Fence> mFence;
		uint64_t fenceVal;

		HANDLE waitEvent;

		HWND mainWnd;

		std::unordered_map<std::string, std::unique_ptr<GraphicPSO>> mPsos;
		std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaderByteCodes;
		std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mRootSignatures;

		//The z value of the position specifies the order of the drawing
		ShaderLoader mShaderLoader;
		
		
		struct Vertex2D{
			Vector3 Position;
			Vector4 Color;
		};

		struct {
			Mat4x4 Proj;
			ComPtr<ID3D12Resource> mUploadProj;
			void* projBufferWriter;

			std::vector<Vertex2D> vertexList;
			ComPtr<ID3D12Resource> mUploadVertex;
			size_t currVSize;
			void* vertexBufferWriter;
		} Data2D;

		//impelement of draw single mesh;
		struct {
			Mesh* currentMesh;
			ComPtr<ID3D12Resource> ConsPass;
			ComPtr<ID3D12Resource> LightPass;
			
			bool activated;
		} Default3D;
	};
}