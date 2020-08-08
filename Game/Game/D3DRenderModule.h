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
#include "D3DResourceManager.h"
#include "D3DDescriptorHandleAllocator.h"
#include "D3DDrawContext.h"
#include "D3DDataStructure.h"


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
		virtual void polygon2D(const Vector2* verts,int vertNum,Vector4 Color,float depth) override;
		virtual void image2D(Texture& image, Vector2 center, Vector2 size,float rotate, float depth) override;


		virtual void set2DViewPort(Vector2 center,float height) override;

		virtual void bindScene(Scene* scene) override;
		virtual void releaseScene(Scene& scene) override;
		virtual void uploadScene(Scene& scene) override;

		virtual void drawSingleMesh(Mesh& mesh,SceneMaterial* mat) override;
	private:
		void create2DImageDrawCall();

		bool createRenderTargetView();
		void waitForFenceValue(uint64_t fence);

		bool initialize2D();
		bool initializeSingleMesh();
		bool initialize2DImage();
		void draw2D();

		void drawSingleMesh();
		void updateSingleMeshData();
		void initializeSingleMeshData();

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
		
		ShaderLoader mShaderLoader;
		D3DResourceManager mResourceManager;
		D3DDescriptorHandleAllocator mDescriptorAllocator;

		struct {
			Mat4x4 Proj;
			UUID projResource;

			std::vector<Vertex2D> vertexList;
			UUID uploadVertex;
			size_t currVSize;

		} Data2D;

		

		struct {
			std::vector<ImageVertex2D> imageVertexList;
			std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>  images;

			UUID imageUploadVertex;
			size_t currImageVSize;
		} Image2DBuffer;

		struct {
			Scene* currScene;

		} Data3D;

		//this pass can only be used to test whether the mesh data 
		//and render system doing well currently we don't consider 
		//material
		struct {
			Mesh* mesh = nullptr;
			SceneMaterial* material = nullptr;
			ObjectPass objectData;
			UUID gpuTransform;
			float dis;
			
			const int lightPassNum = 1;
			LightPass lightData;
			UUID gpuLightData;
			float lightu;

			CameraPass cameraData;
			UUID gpuCameraData;
		}SingleMesh;


		std::vector<D3DDrawContext*> drawCommandList;

	};
}