#include "D3DRenderModule.h"
#include "WindowsApplication.h"
#include "Common.h"
#include <DirectXColors.h>
#include "Timer.h"
#include "InputBuffer.h"
#include "GeometryGenerator.h"

#include "compile_config.h"

#ifdef ENGINE_MODE_EDITOR
#include "EditorGUIModule.h"
namespace Game {
	extern EditorGUIModule gEditorGUIModule;
}
#endif

using namespace Game;

#ifndef PI
#define PI 3.1415926
#endif

namespace Game {
	extern IApplication* app;
	extern FileLoader* gFileLoader;
	extern Timer gTimer;
	extern InputBuffer gInput;

}

void GetHardwareAdapter(IDXGIFactory4* factory,IDXGIAdapter1** adapter) {
	int i = 1;
	while (factory->EnumAdapters1(i++,adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		(*adapter)->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		else return;
	}

	adapter = nullptr;
	return;
}

bool D3DRenderModule::createRenderTargetView() {
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NodeMask = 0;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NumDescriptors = mBackBufferNum;

	if (FAILED(mDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRtvHeap)) ) ) {
		return false;
	}

	rtvDescriptorIncreament = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i != mBackBufferNum; i++) {
		mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i]));
		
		mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1,rtvDescriptorIncreament);
	}

	D3D12_RESOURCE_DESC depthDesc;
	depthDesc.Format = mDepthBufferFormat;
	depthDesc.Alignment = 0;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthDesc.Height = height;
	depthDesc.Width = width;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.MipLevels = 1;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;

	//We assume that the default depth stencil compare function is less so the 
	//default value of the depth buffer is 1.0f
	D3D12_CLEAR_VALUE cValue = {};
	cValue.Format = mDepthBufferFormat;
	cValue.DepthStencil.Depth = 1.f;
	cValue.DepthStencil.Stencil = 0;

	HRESULT hr = mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&cValue,IID_PPV_ARGS(&mDepthStencilBuffer));

	if (FAILED(hr)) return false;

	D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc;
	depthHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	depthHeapDesc.NodeMask = 0;
	depthHeapDesc.NumDescriptors = 1;
	depthHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	hr = mDevice->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&dsvHeap));

	if (FAILED(hr)) return false;

	mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(),
		nullptr,dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void D3DRenderModule::waitForFenceValue(uint64_t fence) {
	mCmdQueue->Signal(mFence.Get(), fence);
	if (mFence->GetCompletedValue() < fence) {
		mFence->SetEventOnCompletion(fence, waitEvent);
		WaitForSingleObject(waitEvent, INFINITE);
	}
}

bool D3DRenderModule::initialize() {
	#ifdef _DEBUG 
	{
		ComPtr<ID3D12Debug> debug;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		debug->EnableDebugLayer();
		debug->Release();
	}
	#endif

	#define HR_FAIL(hr,log) if(FAILED(hr)){\
		Log(log"\n");\
		return false;\
	}

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory));
	HR_FAIL(hr, "d3d12 error : fail to create dxgi factory");

	ComPtr<IDXGIAdapter1> adapter;
	GetHardwareAdapter(mDxgiFactory.Get(),&adapter);

	if (adapter == nullptr) {
		Log("d3d12 error : no hardware adapter on this device\n");
		return false;
	}
	else {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		Log("====================================\n"
			"d3d12 current device attributes :\n"
			"%ls ,\n"
			"%.2f GB video memory,\n"
			"%.2f GB shared system memory\n"
			"====================================\n",
			desc.Description,
			desc.DedicatedVideoMemory / 1073741824.,
			desc.SharedSystemMemory / 1073741824.);
	
	}

	hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
	HR_FAIL(hr, "d3d12 error : fail to create device");

	mResourceManager.initialize(mDevice.Get());

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = mDevice->CreateCommandQueue(&desc,IID_PPV_ARGS(&mCmdQueue));
	if (FAILED(hr)) {
		Log("d3d12 error : fail to create command queue\n");
		return false;
	}


//-------------------------//
	hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&mCmdAllc));
	HR_FAIL(hr, "d3d12 error : fail to create command allocator");

	hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCmdAllc.Get(),nullptr,IID_PPV_ARGS(&mCmdList));
	HR_FAIL(hr, "d3d12 error : fail to create command list");

	mCmdList->Close();
//-------------------------//


	WindowsApplication* wapp = dynamic_cast<WindowsApplication*>(app);

	IDXGISwapChain1* sc1;

	DXGI_SWAP_CHAIN_DESC1 scDesc;
	ZeroMemory(&scDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	scDesc.BufferCount = mBackBufferNum;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scDesc.Format = mBackBufferFormat;
	scDesc.Width = width = wapp->getSysConfig().width;
	scDesc.Height = height = wapp->getSysConfig().height;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.Stereo = FALSE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	winMain = wapp->getMainWnd();

	hr = mDxgiFactory->CreateSwapChainForHwnd(
		mCmdQueue.Get(),
		winMain,
		&scDesc,
		nullptr,
		nullptr,
		&sc1
	);

	HR_FAIL(hr, "d3d12 error : fail to create swap chain");

	mSwapChain.Attach(reinterpret_cast<IDXGISwapChain3*>(sc1));
	mCurrentFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	if (!createRenderTargetView()) {
		Log("d3d12 error : fail to create render target views\n");
		return false;
	}

	fenceVal = 0;
	hr = mDevice->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence));
	HR_FAIL(hr,"d3d12 error : fail to create fence");

	waitEvent = CreateEventEx(nullptr, L"", NULL, EVENT_ALL_ACCESS);

	if (!this->mDescriptorAllocator.initialize(mDevice.Get())) {
		Log("d3d12 error : fail to initialize descriptor handle allocator");
		return false;
	}

	if (!initialize2D()) {
		Log("d3d12 error : fail to initialize 2D render module\n");
		return false;
	}


	if (!initialize2DImage()) {
		Log("d3d12 error : fail to initialize 2D image render module\n");
		return false;
	}

	if (!initialize3D()) {
		Log("d3d12 error : fail to initialize 3D default render pipeline\n");
		return false;
	}
	
	if (!initializeSkyBox()) {
		Log("d3d12 error : fail to initialize sky box render pipeline\n");
		return false;
	}

	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1.0f;

	sissorRect.bottom = height;
	sissorRect.top = 0;
	sissorRect.left = 0;
	sissorRect.right = width;
#ifdef _DEBUG
	if (!initializeSingleMesh()) {
		Log("d3d12 error : fail to initialize 3D default render module\n");
		return false;
	}

	SingleMesh.gpuLightData = UUID::invalid;
	SingleMesh.gpuCameraData = UUID::invalid;
	SingleMesh.gpuTransform = UUID::invalid;
#endif // DEBUG

	Image2DBuffer.currImageVSize = 0;
	Image2DBuffer.imageUploadVertex = UUID::invalid;

	GpuStallEvent = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
	if (!GpuStallEvent) {
		Log("d3d12 error : fail to create gpu event\n");
		return false;
	}
	isInitialized = true;

	return true;
}


void D3DRenderModule::FlushCommandQueue() {
	fenceVal++;
	mCmdQueue->Signal(mFence.Get(), fenceVal);

	if (mFence->GetCompletedValue() < fenceVal) {
		if (FAILED(mFence->SetEventOnCompletion(fenceVal,GpuStallEvent))) {
			Log("d3d12 error : fail to set event on completion system will quit\n");
			dynamic_cast<WindowsApplication*>(app)->Quit();
			return;
		}

		WaitForSingleObject(GpuStallEvent, INFINITE);
	}

}

void D3DRenderModule::tick() {
	//------------------------------------//

#ifdef _DEBUG
	updateSingleMeshData();
#endif // _DEBUG


	mCmdAllc->Reset();
	mCmdList->Reset(mCmdAllc.Get(), nullptr);

	mCmdList->RSSetScissorRects(1, &sissorRect);
	mCmdList->RSSetViewports(1, &viewPort);

	mCmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(
		mBackBuffers[mCurrentFrameIndex].Get(),D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	mResourceManager.tick(mCmdList.Get());

	create2DImageDrawCall();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(mCurrentFrameIndex,rtvDescriptorIncreament);

	mCmdList->OMSetRenderTargets(1, &rtvHandle,TRUE,
		&dsvHeap->GetCPUDescriptorHandleForHeapStart());

	mCmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue,
		0,nullptr);
	mCmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	//drawing 2d objects
	draw2D();

#ifdef _DEBUG
	//draw single pass object
	drawSingleMesh();
#endif


	for (auto& cmd : drawCommandList) {
		cmd->BindOnCommandList(mCmdList.Get());
		gMemory.Delete(cmd);
	}
	drawCommandList.clear();

	drawSkyBox();

	//draw the gui items
#ifdef _WIN64
	gEditorGUIModule.draw(mCmdList.Get());
#endif

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		mBackBuffers[mCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	));

	mCmdList->Close();

	ID3D12CommandList* toExcute[] = {mCmdList.Get()};
	mCmdQueue->ExecuteCommandLists(_countof(toExcute),toExcute);

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mBackBufferNum;

	if (FAILED(mSwapChain->Present(0, 0))) {
		Log("fail to present swap chain\n");
		dynamic_cast<WindowsApplication*>(app)->Quit();
		return;
	}

	FlushCommandQueue();
	//------------------------------------//
}


void D3DRenderModule::finalize() {
	waitForFenceValue(fenceVal);

	mDxgiFactory = nullptr;

	mSwapChain = nullptr;
	mDevice = nullptr;

	for (int i = 0; i != mBackBufferNum; i++)
		mBackBuffers[i] = nullptr;

	mCmdQueue = nullptr;

	//--------------------//
	mCmdAllc = nullptr;
	mCmdList = nullptr;
	//--------------------//

	mFence = nullptr;
	/*
	Data2D.mUploadProj->Unmap(0,nullptr);
	if(Data2D.mUploadVertex)
		Data2D.mUploadVertex->Unmap(0,nullptr);
	*/

	mResourceManager.finalize();
	CloseHandle(waitEvent);
}

void D3DRenderModule::point2D(Vector2 pos,float width, Vector4 Color,float depth) {
	width = width / 2.;
	
	// The upper triangle
	Vertex2D p = {Vector3(pos.x - width,pos.y + width,depth),Color};
	Data2D.vertexList.push_back(p);
	p.Position = Vector3(pos.x + width, pos.y + width, depth);
	Data2D.vertexList.push_back(p);
	p.Position = Vector3(pos.x - width, pos.y - width, depth);
	Data2D.vertexList.push_back(p);

	//The lower triangle
	p.Position = Vector3(pos.x + width, pos.y + width, depth);
	Data2D.vertexList.push_back(p);
	p.Position = Vector3(pos.x + width, pos.y - width, depth);
	Data2D.vertexList.push_back(p);
	p.Position = Vector3(pos.x - width, pos.y - width, depth);
	Data2D.vertexList.push_back(p);

}

void D3DRenderModule::line2D(Vector2 start,Vector2 end,float width, Vector4 Color,float depth) {
	width = width / 2.;

	Vector2 dir = normalize(start - end), vert = Vector2(- dir.y,dir.x);

	Vector2 p0 = start + vert * width, p1 = end + vert * width,
		p2 = start - vert * width, p3 = end - vert * width;

	Vertex2D p;
#define PUSH_BACK_VERTEX_(vec) p = {Vector3(vec.x,vec.y,depth),Color};Data2D.vertexList.push_back(p)

	//The upper triangle
	PUSH_BACK_VERTEX_(p2);
	PUSH_BACK_VERTEX_(p1);
	PUSH_BACK_VERTEX_(p0);

	//The lower triangle
	PUSH_BACK_VERTEX_(p1);
	PUSH_BACK_VERTEX_(p2);
	PUSH_BACK_VERTEX_(p3);
}

void D3DRenderModule::polygon2D(const Vector2* verts,int vertNum,Vector4 color,float depth) {
	//nothing to draw if the polygon only have less than 3 edges
	if (vertNum < 3) return;
	
	for (int i = 1; i != vertNum - 1; i++) {
		Data2D.vertexList.push_back({ Vector3(verts[0].x,verts[0].y,depth),color });
		Data2D.vertexList.push_back({ Vector3(verts[i].x,verts[i].y,depth),color });
		Data2D.vertexList.push_back({ Vector3(verts[i + 1].x,verts[i + 1].y,depth),color });
	}
}

void D3DRenderModule::cricle2D(Vector2 pos, float radius, Vector4 Color, float depth, int powPoly) {
	int edgeNum = 1 << powPoly;
	float rStep = PI * 2. / (float)edgeNum;
	std::vector<Vector3> vPos;
	Vector3 _Pos = { pos.x,pos.y,depth };
	for (int i = 0; i != edgeNum; i++) {
		vPos.push_back(_Pos + Vector2(sin(rStep * (float)i), cos(rStep * (float)i)) * radius);
	}

	for (int i = 0; i <= powPoly - 2; i++) {
		int stepSize = 1 << i, marchSize = stepSize << 1;
		for (int j = 0; j != edgeNum; j += marchSize) {
			Data2D.vertexList.push_back({ vPos[j],Color });
			Data2D.vertexList.push_back({ vPos[j + stepSize],Color });
			Data2D.vertexList.push_back({ vPos[(j + stepSize * 2) % edgeNum],Color });
		}
	}
}


void D3DRenderModule::set2DViewPort(Vector2 center,float _height) {
	float _width = _height * (float)width / (float)height;
	
	Data2D.Proj = Mat4x4(
		2. /_width,0          ,0,- 2. * center.x /_width ,
		0         ,2. /_height,0,- 2. * center.y /_height,
		0         ,0          ,1,0                       ,
		0         ,0          ,0,1
	).T();

	//memcpy(Data2D.projBufferWriter, &Data2D.Proj, sizeof(Data2D.Proj));
	mResourceManager.write(Data2D.projResource,&Data2D.Proj,sizeof(Data2D.Proj));
}

bool D3DRenderModule::initialize2D() {

	//The shader have not been compiled.Find the source code and compile them 
	Buffer shader2D;
	ComPtr<ID3DBlob> VS2D, PS2D;

	VS2D = mShaderLoader.CompileFile("Geometry2D.hlsl", "VS", VS);
	PS2D = mShaderLoader.CompileFile("Geometry2D.hlsl", "PS", PS);
	
	//the name of the 2d vertex shader is Game2D_VS and 
	//the name of the 2d pixel shadere is Game2D_PS while
	//the system defined shaders' name start with 'Game'
	mShaderByteCodes["Game2D_VS"] = VS2D;
	mShaderByteCodes["Game2D_PS"] = PS2D;

	D3D12_INPUT_ELEMENT_DESC desc;
	
	//The input layout corresponding to the shader Game2D
	InputLayout layout = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	RootSignature rootSig(1,0);
	rootSig[0].initAsConstantBuffer(0,0);
	if (!rootSig.EndEditingAndCreate(mDevice.Get())) {
		Log("fail to create rootsignature");
		return false;
	}
	mRootSignatures["Game2D"] = rootSig.GetRootSignature();

	mPsos["Game2D"] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso2D = mPsos["Game2D"].get();

	pso2D->LazyBlendDepthRasterizeDefault();
	pso2D->SetDepthStencilViewFomat(mDepthBufferFormat);
	pso2D->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	pso2D->SetInputElementDesc(layout);
	pso2D->SetNodeMask(0);

	pso2D->SetPixelShader(PS2D->GetBufferPointer(),PS2D->GetBufferSize());
	pso2D->SetVertexShader(VS2D->GetBufferPointer(),VS2D->GetBufferSize());

	pso2D->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso2D->SetRenderTargetFormat(mBackBufferFormat);
	pso2D->SetRootSignature(&rootSig);
	pso2D->SetSampleDesc(1, 0);
	pso2D->SetSampleMask(UINT_MAX);

	if (!pso2D->Create(mDevice.Get())) {
		Log("fail to create Pipeline State Object for 2D rendering\n");
		return false;
	}


	//initialize the data part
	Data2D.Proj = Mat4x4::I();

	Data2D.projResource = mResourceManager.uploadDynamic(&Data2D.Proj, sizeof(Data2D.Proj), 
		CD3DX12_RESOURCE_DESC::Buffer(sizeof(Data2D.Proj)),
		true);

	Data2D.currVSize = 0;
	//Data2D.mUploadVertex = nullptr;
	return true;
}

void D3DRenderModule::draw2D() {
	//update data
	//if the current resource buffer's size is smaller than the 
	//vertex data in main memory.Then create a resource block 
	//2 times the main memory requires

	if (!Data2D.vertexList.size()) return;

	size_t vSize = Data2D.vertexList.size() * sizeof(Vertex2D);

	//draw 2d items
	if (Data2D.currVSize < Data2D.vertexList.size()) {
		mResourceManager.releaseResource(Data2D.uploadVertex);
		Data2D.currVSize = Data2D.vertexList.size() * 2;

		Data2D.uploadVertex = mResourceManager.uploadDynamic(Data2D.vertexList.data(), vSize,
			CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex2D) * Data2D.currVSize));
	}
	else {
		mResourceManager.write(Data2D.uploadVertex,Data2D.vertexList.data(), vSize);
	}

	mCmdList->SetPipelineState(mPsos["Game2D"]->GetPSO());
	mCmdList->SetGraphicsRootSignature(mRootSignatures["Game2D"].Get());

	mCmdList->SetGraphicsRootConstantBufferView(0,
		mResourceManager.getResource(Data2D.projResource)->GetGPUVirtualAddress());

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = mResourceManager.getResource(Data2D.uploadVertex)->GetGPUVirtualAddress();
	vbv.SizeInBytes = vSize;
	vbv.StrideInBytes = sizeof(Vertex2D);

	mCmdList->IASetVertexBuffers(0 , 1 , &vbv);
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCmdList->DrawInstanced(Data2D.vertexList.size(), 1, 0, 0);

	

	//after every frame clear up the vertex buffer
	Data2D.vertexList.clear();

}


void D3DRenderModule::releaseScene(Scene& scene) {
	for (auto& item : scene.meshs) {
		mResourceManager.releaseResource(item.second.gpuDataToken.index);
		mResourceManager.releaseResource(item.second.gpuDataToken.vertex);
	}

	for (auto& item : scene.textures) {
		mResourceManager.releaseResource(item.second.gpuDataToken);
	}

	/*
	for (auto& item : scene.buffers) {
		mResourceManager.releaseResource(item.second.gpuDataToken);
	}
	*/
}



void D3DRenderModule::uploadScene(Scene& scene) {
	for (auto& item : scene.meshs) {
		//if the mesh data uses index buffer data and the index data have never been uploaded
		if (item.second.gpuDataToken.index == UUID::invalid
			&& item.second.useIndexList) {
			item.second.gpuDataToken.index =
				mResourceManager.uploadStaticBuffer(item.second.indexList, D3D12_RESOURCE_STATE_COMMON);
		}
		if (item.second.gpuDataToken.vertex == UUID::invalid) {
			item.second.gpuDataToken.vertex =
				mResourceManager.uploadStaticBuffer(item.second.vertexList, D3D12_RESOURCE_STATE_COMMON);
		}
	}

	for (auto& item : scene.textures) {
		if (item.second.gpuDataToken == UUID::invalid) {
			item.second.gpuDataToken =
				mResourceManager.uploadTexture(item.second, D3D12_RESOURCE_STATE_COMMON);
		}
	}

	/*
	for (auto& item : scene.buffers) {
		//constant buffers need to align to 256
		if (item.second.gpuDataToken == UUID::invalid) {
			item.second.gpuDataToken =
				mResourceManager.uploadStaticBuffer(item.second.buffer,
					D3D12_RESOURCE_STATE_COMMON,
					true);
		}
	}
	*/
}



void D3DRenderModule::image2D(Texture& image,Vector2 center,Vector2 size,float rotate,float depth) {
	if (image.type != TEXTURE_TYPE::TEXTURE2D) {
		Log("d3d12::image2D error : image2D only accept and draw texture whose type is TEXTURE2D\n");
		return;
	}

	if (image.gpuDataToken == UUID::invalid) {
		image.gpuDataToken = mResourceManager.uploadTexture(image,
			D3D12_RESOURCE_STATE_COMMON);
		if (image.gpuDataToken == UUID::invalid) {
			Log("d3d12::image2D fail to upload image data");	
		}
		return;
	}

 	D3DDescriptorHandle srvHandle = mDescriptorAllocator.getDescriptorHandle(image.gpuDataToken,
										D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srvHandle == D3DDescriptorHandle::invalid) {
		ID3D12Resource* texture = mResourceManager.getResource(image.gpuDataToken);
		if (texture == nullptr) {
			//Log("d3d12 : unexpected error occurs during upload 2d image,can't find handle %s 's corresponding resource\n",
			//	image.gpuDataToken.to_string().c_str());
			return;
		}
		srvHandle = mDescriptorAllocator.AllocateNewHandle(image.gpuDataToken,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (srvHandle == D3DDescriptorHandle::invalid) {
			Log("d3d12 : unexpected error occurs during upload 2d image,can't allocate new descriptor handle for new resource\n");
			return;
		}
		

		D3D12_RESOURCE_DESC rDesc = texture->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = rDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		
		srvDesc.Texture2D.MipLevels = rDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

		mDevice->CreateShaderResourceView(texture,&srvDesc,srvHandle.cpuHandle);
	}

	Image2DBuffer.textures.push_back(srvHandle.gpuHandle);
	Image2DBuffer.images_num.push_back(1);

	float c = cos(rotate), s = sin(rotate);
	Mat2x2 Rotate(c, s, -s, c);

	Vector2 lt = mul(Rotate, Vector2(-size.x, size.y)),
		rt = mul(Rotate, size);

	//    ------ need to be fixed ---- //
	ImageVertex2D v[4];
	Vector2 Pos = center + lt / 2.;
	v[0].Position = Vector3(Pos.x, Pos.y, depth);
	v[0].Texcoord = Vector2(0., 1.);
	Pos = center + rt / 2.;
	v[1].Position = Vector3(Pos.x, Pos.y, depth);
	v[1].Texcoord = Vector2(1., 1.);
	Pos = center - lt / 2.;
	v[2].Position = Vector3(Pos.x, Pos.y, depth);
	v[2].Texcoord = Vector2(1., 0.);
	Pos = center - rt / 2.;
	v[3].Position = Vector3(Pos.x, Pos.y, depth);
	v[3].Texcoord = Vector2(0., 0.);

	Image2DBuffer.imageVertexList.push_back(v[0]);
	Image2DBuffer.imageVertexList.push_back(v[1]);
	Image2DBuffer.imageVertexList.push_back(v[2]);
	Image2DBuffer.imageVertexList.push_back(v[0]);
	Image2DBuffer.imageVertexList.push_back(v[2]);
	Image2DBuffer.imageVertexList.push_back(v[3]);
}

void D3DRenderModule::image2D(Texture& image,Vector2* center,Vector2* size,float* rotate,float* depth,size_t num) {
	if (image.type != TEXTURE_TYPE::TEXTURE2D) {
		Log("d3d12::image2D error : image2D only accept and draw texture whose type is TEXTURE2D\n");
		return;
	}

	if (image.gpuDataToken == UUID::invalid) {
		image.gpuDataToken = mResourceManager.uploadTexture(image,
			D3D12_RESOURCE_STATE_COMMON);
		if (image.gpuDataToken == UUID::invalid) {
			Log("d3d12::image2D fail to upload image data");
		}
		return;
	}

	D3DDescriptorHandle srvHandle = mDescriptorAllocator.getDescriptorHandle(image.gpuDataToken,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srvHandle == D3DDescriptorHandle::invalid) {
		ID3D12Resource* texture = mResourceManager.getResource(image.gpuDataToken);
		if (texture == nullptr) {
			//Log("d3d12 : unexpected error occurs during upload 2d image,can't find handle %s 's corresponding resource\n",
			//	image.gpuDataToken.to_string().c_str());
			return;
		}
		srvHandle = mDescriptorAllocator.AllocateNewHandle(image.gpuDataToken,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (srvHandle == D3DDescriptorHandle::invalid) {
			Log("d3d12 : unexpected error occurs during upload 2d image,can't allocate new descriptor handle for new resource\n");
			return;
		}


		D3D12_RESOURCE_DESC rDesc = texture->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = rDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		srvDesc.Texture2D.MipLevels = rDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

		mDevice->CreateShaderResourceView(texture, &srvDesc, srvHandle.cpuHandle);
	}

	Image2DBuffer.textures.push_back(srvHandle.gpuHandle);
	Image2DBuffer.images_num.push_back(num);

	for (size_t i = 0; i != num ; i++) {
		Vector2 _center = center[i],
			_size = size[i];
		float _depth = depth[i], _rotate = rotate[i];

		float c = cos(_rotate), s = sin(_rotate);
		Mat2x2 Rotate(c, s, -s, c);

		Vector2 lt = mul(Rotate, Vector2(-_size.x, _size.y)),
			rt = mul(Rotate, _size);

		//    ------ need to be fixed ---- //
		ImageVertex2D v[4];
		Vector2 Pos = _center + lt / 2.;
		v[0].Position = Vector3(Pos.x, Pos.y, _depth);
		v[0].Texcoord = Vector2(0., 1.);
		Pos = _center + rt / 2.;
		v[1].Position = Vector3(Pos.x, Pos.y, _depth);
		v[1].Texcoord = Vector2(1., 1.);
		Pos = _center - lt / 2.;
		v[2].Position = Vector3(Pos.x, Pos.y, _depth);
		v[2].Texcoord = Vector2(1., 0.);
		Pos = _center - rt / 2.;
		v[3].Position = Vector3(Pos.x, Pos.y, _depth);
		v[3].Texcoord = Vector2(0., 0.);

		Image2DBuffer.imageVertexList.push_back(v[0]);
		Image2DBuffer.imageVertexList.push_back(v[1]);
		Image2DBuffer.imageVertexList.push_back(v[2]);
		Image2DBuffer.imageVertexList.push_back(v[0]);
		Image2DBuffer.imageVertexList.push_back(v[2]);
		Image2DBuffer.imageVertexList.push_back(v[3]);
	}
}

bool D3DRenderModule::initialize2DImage() {
	ComPtr<ID3DBlob> VS, PS;
	RootSignature rootSig(2,1);

	//to upload the texture we need to initialize the root signature as a SRV descriptor table
	//here is the microsoft d3d document says
	//The inlined root descriptors should contain descriptors that are accessed most often, 
	//though is limited to CBVs, and raw or structured UAV or SRV buffers. A more complex type, 
	//such as a 2D texture SRV, cannot be used as a root descriptor. 
	rootSig[0].initAsDescriptorTable(0,0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	rootSig.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));
	rootSig[1].initAsConstantBuffer(0, 0);

	if (!rootSig.EndEditingAndCreate(mDevice.Get())) {
		Log("d3d12 : fail to create 2d image pipeline");
		return false;
	}

	mRootSignatures["Image2D"] = rootSig.GetRootSignature();

	VS = mShaderLoader.CompileFile("Image2D.hlsl", "VS", SHADER_TYPE::VS);
	PS = mShaderLoader.CompileFile("Image2D.hlsl", "PS", SHADER_TYPE::PS);

	if (VS == nullptr || PS == nullptr) {
		return false;
	}

	InputLayout input = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	mShaderByteCodes["Image2DVS"] = VS;
	mShaderByteCodes["Image2DPS"] = PS;

	mPsos["Image2D"] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso = mPsos["Image2D"].get();

	pso->LazyBlendDepthRasterizeDefault();
	pso->SetDepthStencilViewFomat(mDepthBufferFormat);
	pso->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);

	pso->SetInputElementDesc(input);
	pso->SetNodeMask(0);

	pso->SetVertexShader(VS->GetBufferPointer(),VS->GetBufferSize());
	pso->SetPixelShader(PS->GetBufferPointer(),PS->GetBufferSize());

	pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso->SetRootSignature(&rootSig);

	pso->SetRenderTargetFormat(mBackBufferFormat);
	pso->SetSampleDesc(1, 0);
	pso->SetSampleMask(UINT_MAX);

	if (!pso->Create(mDevice.Get())) {
		Log("d3d12 : fail to create pipeline state object for image 2d pipeline\n");
		return false;
	}

	return true;
}

void D3DRenderModule::create2DImageDrawCall() {
	if (Image2DBuffer.imageVertexList.empty()) {
		return;
	}

	ID3D12Resource* ProjResource = mResourceManager.getResource(Data2D.projResource);
	if (ProjResource == nullptr) {
		Log("error : fail to get projection matrix resource ,fail to make draw call\n");
		return;
	}
	//if the resource is out of bounary.Release the release the vertex buffer and create a 
	//vertex buffer twice as big as the space required
	size_t requiredVertBufferSize = Image2DBuffer.imageVertexList.size() * sizeof(ImageVertex2D);
	if (requiredVertBufferSize >= Image2DBuffer.currImageVSize) {
		mResourceManager.releaseResource(Image2DBuffer.imageUploadVertex);

		Image2DBuffer.imageUploadVertex = mResourceManager.uploadDynamic(Image2DBuffer.imageVertexList.data(),
			requiredVertBufferSize, CD3DX12_RESOURCE_DESC::Buffer(requiredVertBufferSize * 2));
	}
	else {
		mResourceManager.write(Image2DBuffer.imageUploadVertex,
			Image2DBuffer.imageVertexList.data(), requiredVertBufferSize);
	}

	ID3D12Resource* mVertResource = mResourceManager.getResource(Image2DBuffer.imageUploadVertex);

	size_t offset = 0;
	for (size_t i = 0; i != Image2DBuffer.textures.size(); i++) {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mVertResource->GetGPUVirtualAddress() + offset;
		vbv.SizeInBytes = sizeof(ImageVertex2D) * 6 * Image2DBuffer.images_num[i];
		vbv.StrideInBytes = sizeof(ImageVertex2D);

		offset += sizeof(ImageVertex2D) * 6 * Image2DBuffer.images_num[i] ;
		
		D3DDrawContext* draw = gMemory.New<D3DDrawContext>(mPsos["Image2D"]->GetPSO(), mRootSignatures["Image2D"].Get(),
			mDescriptorAllocator.getDescriptorHeap(),2, vbv, 6 * Image2DBuffer.images_num[i]);
		
		draw->SetShaderParameter(1, D3DShaderParameter(ProjResource->GetGPUVirtualAddress(),
			D3D12_ROOT_PARAMETER_TYPE_CBV, 1));
		draw->SetShaderParameter(0, D3DShaderParameter(Image2DBuffer.textures[i],
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, 0));

		drawCommandList.push_back(draw);
	}

	Image2DBuffer.textures.clear();
	Image2DBuffer.images_num.clear();
	Image2DBuffer.imageVertexList.clear();
}

#ifdef _DEBUG
//initialize the default render pipeline
bool D3DRenderModule::initializeSingleMesh() {

	//The shader have not been compiled.Find the source code and compile them 
	Buffer shader;
	ComPtr<ID3DBlob> VS, PS;

	VS = mShaderLoader.CompileFile("DefaultMat.hlsl", "VS", SHADER_TYPE::VS);
	PS = mShaderLoader.CompileFile("DefaultMat.hlsl", "PS", SHADER_TYPE::PS);

	if (VS == nullptr || PS == nullptr)
		return false;
	
	mShaderByteCodes["DefaultVS"] = VS;
	mShaderByteCodes["DefaultPS"] = PS;


	RootSignature rootSig(3,0);
	rootSig[0].initAsConstantBuffer(0, 0);
	rootSig[1].initAsConstantBuffer(1, 0);
	rootSig[2].initAsConstantBuffer(2, 0);

	if (!rootSig.EndEditingAndCreate(mDevice.Get())) {
		Log("d3d12 : fail to initialize default render pass : fail to create root signature\n");
		return false;
	}

	mRootSignatures["Default"] = rootSig.GetRootSignature(); 
	
	this->mPsos["Default"] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso = this->mPsos["Default"].get();

	pso->LazyBlendDepthRasterizeDefault();
	

	InputLayout layout = {
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	pso->SetInputElementDesc(layout);
	pso->SetDepthStencilViewFomat(mDepthBufferFormat);
	pso->SetRenderTargetFormat(mBackBufferFormat);
	pso->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso->SetRootSignature(&rootSig);

	pso->SetVertexShader(VS->GetBufferPointer(),VS->GetBufferSize());
	pso->SetPixelShader(PS->GetBufferPointer(),PS->GetBufferSize());

	pso->SetNodeMask(0);
	pso->SetSampleDesc(1, 0);
	pso->SetSampleMask(UINT_MAX);

	if (!pso->Create(mDevice.Get())) {
		Log("d3d12 : fail to initialize default render pipeline : fail to create PSO \n");
		return false;
	}

	
	return true;
}

void D3DRenderModule::updateSingleMeshData() {

	if (gInput.KeyHold(InputBuffer::W)) 
		SingleMesh.dis = min(SingleMesh.dis + 0.05, 18.);
	else if (gInput.KeyHold(InputBuffer::S))
		SingleMesh.dis = max(SingleMesh.dis - 0.05,2.5);
	if (gInput.KeyHold(InputBuffer::A))
		SingleMesh.lightu -= 0.005 * PI;
	else if (gInput.KeyHold(InputBuffer::D))
		SingleMesh.lightu += 0.005 * PI;

	SingleMesh.lightData.Light[0].lightDirection =
		Vector3(sin(SingleMesh.lightu), 0., cos(SingleMesh.lightu));

	Mat4x4 rotate = MatrixRotateY( PI * 0.1 * gTimer.TotalTime());//(reinterpret_cast<float*>(&mat));

	Mat4x4 world = MatrixPosition(Vector3(0, 0, SingleMesh.dis));
	world = mul(world, rotate);
	SingleMesh.objectData.world =  world.T();
	SingleMesh.objectData.transInvWorld = world.R().T();
	mResourceManager.write(SingleMesh.gpuTransform, &SingleMesh.objectData,
		sizeof(ObjectPass));
	mResourceManager.write(SingleMesh.gpuLightData,&SingleMesh.lightData,sizeof(SingleMesh.lightData));
}

void D3DRenderModule::drawSingleMesh() {
	if (!SingleMesh.mesh) return;
	//if the single mesh haven't upload mesh data to the resource manager
	//try to upload the data
	if (SingleMesh.mesh->gpuDataToken.vertex == UUID::invalid) {
		SingleMesh.mesh->gpuDataToken.vertex = mResourceManager.uploadStaticBuffer(
			SingleMesh.mesh->vertexList, D3D12_RESOURCE_STATE_COMMON);
		if (SingleMesh.mesh->gpuDataToken.index == UUID::invalid &&
				SingleMesh.mesh->useIndexList) {
			SingleMesh.mesh->gpuDataToken.index = mResourceManager.uploadStaticBuffer(
				SingleMesh.mesh->indexList,D3D12_RESOURCE_STATE_COMMON);
		}
		return;
	}
	
	ID3D12Resource* vertData = mResourceManager.getResource(SingleMesh.mesh->gpuDataToken.vertex);
	//printf("%s \n",to_string(*SingleMesh.mesh).c_str());

	if (!vertData) {
		Log("d3d 12 draw single mesh : can't fetch vertex data for some reason\n");
		return;
	}
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertData->GetGPUVirtualAddress();
	vbv.SizeInBytes = SingleMesh.mesh->vertexList.size;
	vbv.StrideInBytes = Mesh::vertexSize;

	mCmdList->SetPipelineState(mPsos["Default"]->GetPSO());
	
	mCmdList->SetGraphicsRootSignature(mRootSignatures["Default"].Get());

	mCmdList->SetGraphicsRootConstantBufferView(0,
		mResourceManager.getResource(SingleMesh.gpuTransform)->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(1,
		mResourceManager.getResource(SingleMesh.gpuCameraData)->GetGPUVirtualAddress());
	mCmdList->SetGraphicsRootConstantBufferView(2,
		mResourceManager.getResource(SingleMesh.gpuLightData)->GetGPUVirtualAddress());

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->IASetVertexBuffers(0, 1, &vbv);

	if (SingleMesh.mesh->useIndexList) {
		ID3D12Resource* indexData = mResourceManager.getResource(SingleMesh.mesh->gpuDataToken.index);

		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = indexData->GetGPUVirtualAddress();
		ibv.Format = SingleMesh.mesh->indexType == Mesh::I16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		ibv.SizeInBytes = SingleMesh.mesh->indexList.size;

		mCmdList->IASetIndexBuffer(&ibv);
		mCmdList->DrawIndexedInstanced(SingleMesh.mesh->indexNum,
			1, 0, 0, 0);
	}
	else
		mCmdList->DrawInstanced(SingleMesh.mesh->vertexNum, 1, 0, 0);

}

void D3DRenderModule::initializeSingleMeshData() {

	SingleMesh.dis = 9.;
	Mat4x4 world = MatrixPosition(Vector3(0,0,SingleMesh.dis));
	world = mul(world,MatrixRotateY(PI));
	SingleMesh.objectData.world = world.T();
	SingleMesh.objectData.transInvWorld = world.R();
	if (SingleMesh.gpuTransform == UUID::invalid) {
		SingleMesh.gpuTransform =
			mResourceManager.uploadDynamic(&SingleMesh.objectData,
				sizeof(ObjectPass), CD3DX12_RESOURCE_DESC::Buffer(sizeof(ObjectPass)),true);
	}

	SingleMesh.lightu = 0;
	SingleMesh.lightData.Light[0].lightDirection = 
		Vector3(sin(SingleMesh.lightu),0.,cos(SingleMesh.lightu));
	SingleMesh.lightData.Light[0].lightIntensity = Vector4(0.75,0.75,0.75,1);
	SingleMesh.lightData.ambient = Vector3(0.25,0.25,0.25);
	if (SingleMesh.gpuLightData == UUID::invalid) {
		SingleMesh.gpuLightData =
			mResourceManager.uploadDynamic(&SingleMesh.lightData,
				sizeof(LightPass), CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightPass)), true);
	}

	Mat4x4 proj = MatrixProjection((float)width / (float)height, PI / 4.);
	SingleMesh.cameraData.projection = proj.T();
	SingleMesh.cameraData.invProjection = proj.R().T();
	if (SingleMesh.gpuCameraData == UUID::invalid) {
		SingleMesh.gpuCameraData =
			mResourceManager.uploadDynamic(&SingleMesh.cameraData,
				sizeof(CameraPass), CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraPass)), true);
	}

}


//currently we don't consider custom material
void D3DRenderModule::drawSingleMesh(Mesh& mesh, Material* mat) {
	initializeSingleMeshData();
	SingleMesh.mesh = &mesh;
}

void Game::D3DRenderModule::drawSingleScene(Scene* scene) {

}
#endif

bool D3DRenderModule::initialize3D() {
	ComPtr<ID3DBlob> VS3D, PS3D;

	VS3D = mShaderLoader.CompileFile("Default3D.hlsl", "VS", SHADER_TYPE::VS);
	PS3D = mShaderLoader.CompileFile("Default3D.hlsl", "PS", SHADER_TYPE::PS);

	if (VS3D == nullptr || PS3D == nullptr) {
		return false;
	}

	mShaderByteCodes["Default3D_VS"] = VS3D;
	mShaderByteCodes["Default3D_PS"] = PS3D;

	RootSignature rootSig(3, 0);
	rootSig[0].initAsConstantBuffer(0, 0);
	rootSig[1].initAsConstantBuffer(1, 0);
	rootSig[2].initAsConstantBuffer(2, 0);

	rootSig.EndEditingAndCreate(mDevice.Get());


	mRootSignatures["Default3D"] = rootSig.GetRootSignature();

	InputLayout layout = {
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	mPsos["Default3D"] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso = mPsos["Default3D"].get();

	pso->LazyBlendDepthRasterizeDefault();

	pso->SetInputElementDesc(layout);
	pso->SetDepthStencilViewFomat(mDepthBufferFormat);
	pso->SetRenderTargetFormat(mBackBufferFormat);
	pso->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso->SetRootSignature(&rootSig);

	pso->SetVertexShader(VS3D->GetBufferPointer(), VS3D->GetBufferSize());
	pso->SetPixelShader(PS3D->GetBufferPointer(), PS3D->GetBufferSize());

	pso->SetNodeMask(0);
	pso->SetSampleDesc(1, 0);
	pso->SetSampleMask(UINT_MAX);

	if (!pso->Create(mDevice.Get())) {
		Log("d3d12 error : fail to create pipeline state object");
		return false;
	}
	return true;
}

void D3DRenderModule::parseDrawCall(DrawCall* dc,int num) {
	ID3D12DescriptorHeap* heap = mDescriptorAllocator.getDescriptorHeap();
	
	for (int i = 0; i < num; i++) {
		for (int j = 0; j != dc[i].drawTargetNums; j++) {
			Mesh* mesh = dc[i].meshList[j].mesh;
			DrawCall::Parameter* owned_parameter = &dc->OwnedParameterBuffer[j];

			for (int m = 0; m != 3; m++) {
				if (owned_parameter->buffers[m]->uid == UUID::invalid) {
					Buffer* targetData = &owned_parameter->buffers[m]->data;
					owned_parameter->buffers[m]->uid = mResourceManager.uploadDynamic(*targetData,
						CD3DX12_RESOURCE_DESC::Buffer(targetData->size), true);
				}
				else if (owned_parameter->buffers[m]->updated) {
					Buffer* targetData = &owned_parameter->buffers[m]->data;
					mResourceManager.write(owned_parameter->buffers[m]->uid, targetData->data,
						targetData->size);
					owned_parameter->buffers[m]->updated = false;
				}
			}

			if (dc[i].meshList[j].activated) {
				ID3D12Resource* vertexRes = mResourceManager.getResource(mesh->gpuDataToken.vertex);

				if (!vertexRes) { continue; }

				D3D12_VERTEX_BUFFER_VIEW vbv;
				vbv.BufferLocation = vertexRes->GetGPUVirtualAddress();
				vbv.SizeInBytes = mesh->vertexSize * mesh->vertexNum;
				vbv.StrideInBytes = mesh->vertexSize;
				bool useDefaultMat;

				ID3D12PipelineState* pso;
				ID3D12RootSignature* rootSig;
				//if the drawcall's shader is default the pipeline state object will be default object
				if (!dc->material) {
					pso = mPsos["Default3D"]->GetPSO();
					rootSig = mRootSignatures["Default3D"].Get();
					useDefaultMat = true;
				}
				else
					return;

				D3DDrawContext* newDrawContext;

				ID3D12Resource* objectConstBuffer = mResourceManager.getResource(owned_parameter->objectBuffer->uid);
				ID3D12Resource* cameraBuffer = mResourceManager.getResource(owned_parameter->cameraBuffer->uid);
				ID3D12Resource* lightBuffer = mResourceManager.getResource(owned_parameter->lightBuffer->uid);

				if (mesh->useIndexList) {
					ID3D12Resource* indexRes = mResourceManager.getResource(mesh->gpuDataToken.index);

					D3D12_INDEX_BUFFER_VIEW ibv;
					ibv.BufferLocation = indexRes->GetGPUVirtualAddress();
					ibv.SizeInBytes = mesh->indexSize * mesh->indexNum;
					ibv.Format = mesh->indexType == Mesh::I32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

					//if the object use default material to render
					newDrawContext = gMemory.New<D3DDrawContext>(pso, rootSig, heap, 3, vbv, mesh->vertexNum,
						ibv, mesh->indexNum);

					newDrawContext->SetShaderParameter(0, D3DShaderParameter(objectConstBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 0));
					newDrawContext->SetShaderParameter(1, D3DShaderParameter(cameraBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 1));
					newDrawContext->SetShaderParameter(2, D3DShaderParameter(lightBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 2));

				}
				else {

					//if the object use default material to render
					newDrawContext = gMemory.New<D3DDrawContext>(pso, rootSig, heap, 3, vbv, mesh->vertexNum);

					newDrawContext->SetShaderParameter(0, D3DShaderParameter(objectConstBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 0));
					newDrawContext->SetShaderParameter(1, D3DShaderParameter(cameraBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 1));
					newDrawContext->SetShaderParameter(2, D3DShaderParameter(lightBuffer->GetGPUVirtualAddress(), D3D12_ROOT_PARAMETER_TYPE_CBV, 2));

				}
				drawCommandList.push_back(newDrawContext);

			}
		}
	}
}


void D3DRenderModule::skybox(Texture& texture,SceneCamera* camera) {
	if (texture.type !=  TEXTURE_TYPE::CUBE) {
		Log("d3d12 ERROR: sky box fail to set skybox, the type of the skybox texture must be CUBE\n");
		return;
	}
	if (!camera) {
		Log("d3d12 ERROR: the sky box must have a corresponding camera\n");
		return;
	}

	SkyBoxBuffer.sky = &texture;
	SkyBoxBuffer.camera = camera;

	SkyBoxBuffer.active = true;

	if (SkyBoxBuffer.sky->gpuDataToken == UUID::invalid) {
		SkyBoxBuffer.sky->gpuDataToken = mResourceManager.uploadCubeTexture(
			texture.width, texture.height, D3D12_RESOURCE_STATE_COMMON, texture.subDataDescriptor
		);
	}
}


bool D3DRenderModule::initializeSkyBox() {
	ComPtr<ID3DBlob> VS, PS;

	VS = mShaderLoader.CompileFile("SkyBox.hlsl", "VS", SHADER_TYPE::VS);
	PS = mShaderLoader.CompileFile("SkyBox.hlsl", "PS", SHADER_TYPE::PS);

	if (VS == nullptr || PS == nullptr) {
		Log("d3d12 ERROR : fail to compile sky box shaders\n");
		return false;
	}

	mShaderByteCodes["SkyBox_VS"] = VS;
	mShaderByteCodes["SkyBox_PS"] = PS;


	RootSignature rootSig(2, 1);
	rootSig.InitializeSampler(0, CD3DX12_STATIC_SAMPLER_DESC(0));
	rootSig[0].initAsConstantBuffer(0, 0);
	rootSig[1].initAsDescriptorTable(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

	if (!rootSig.EndEditingAndCreate(mDevice.Get())) {
		Log("d3d12 ERROR : fail to initialize sky box pass,fail to create the root signature\n");
		return false;
	}

	InputLayout layout = {
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	mRootSignatures["SkyBox"] = rootSig.GetRootSignature();

	mPsos["SkyBox"] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso = mPsos["SkyBox"].get();

	pso->LazyBlendDepthRasterizeDefault();
	
	D3D12_DEPTH_STENCIL_DESC dsdesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	dsdesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	pso->SetDepthStencilState(dsdesc);


	pso->SetInputElementDesc(layout);
	pso->SetDepthStencilViewFomat(mDepthBufferFormat);
	pso->SetRenderTargetFormat(mBackBufferFormat);
	pso->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	pso->SetRootSignature(&rootSig);

	pso->SetVertexShader(VS->GetBufferPointer(), VS->GetBufferSize());
	pso->SetPixelShader(PS->GetBufferPointer(), PS->GetBufferSize());

	pso->SetNodeMask(0);
	pso->SetSampleDesc(1, 0);
	pso->SetSampleMask(UINT_MAX);

	if (!pso->Create(mDevice.Get())) {
		Log("d3d12 ERROR: fail to create sky box pipeline state object\n");
		return false;
	}

	Mesh box = std::move(GeometryGenerator::generateCube(false,1.f));
	SkyBoxBuffer.box = mResourceManager.uploadStaticBuffer(box.vertexList, D3D12_RESOURCE_STATE_COMMON);
	if (SkyBoxBuffer.box == UUID::invalid) {
		Log("d3d12 ERROR: fail to create sky box pipeline,fail to upload box buffer data\n");
		return false;
	}


	SkyBoxBuffer.boxvbv.BufferLocation = 0;
	SkyBoxBuffer.boxvbv.SizeInBytes = box.vertexNum * box.vertexSize;
	SkyBoxBuffer.boxvbv.StrideInBytes = box.vertexSize;

	return true;
}

void D3DRenderModule::drawSkyBox() {
	if (!SkyBoxBuffer.active) return;

	if (SkyBoxBuffer.boxvbv.BufferLocation == 0) {
		ID3D12Resource* boxVertex = mResourceManager.getResource(SkyBoxBuffer.box);
		if (!boxVertex) return;
		SkyBoxBuffer.boxvbv.BufferLocation = boxVertex->GetGPUVirtualAddress();
	}

	D3DDescriptorHandle handle = mDescriptorAllocator.getDescriptorHandle(SkyBoxBuffer.sky->gpuDataToken,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	if (handle == D3DDescriptorHandle::invalid) {
		handle = mDescriptorAllocator.AllocateNewHandle(
			SkyBoxBuffer.sky->gpuDataToken,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		ID3D12Resource* resource = mResourceManager.getResource(SkyBoxBuffer.sky->gpuDataToken);
		if (handle == D3DDescriptorHandle::invalid) {
			Log("d3d12 error: fail to create the sky box descriptor handle,resource id %s\n",
				SkyBoxBuffer.sky->gpuDataToken.to_string().c_str());
			return;
		}

		D3D12_RESOURCE_DESC rdesc = resource->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Format = rdesc.Format;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;


		srv.TextureCube.MipLevels = rdesc.MipLevels;
		srv.TextureCube.MostDetailedMip = 0;
		srv.TextureCube.ResourceMinLODClamp = 0;

		mDevice->CreateShaderResourceView(resource, &srv, handle.cpuHandle);
	}


	mCmdList->SetPipelineState(mPsos["SkyBox"]->GetPSO());
	mCmdList->SetGraphicsRootSignature(mRootSignatures["SkyBox"].Get());

	ID3D12DescriptorHeap* heaps[1] = {mDescriptorAllocator.getDescriptorHeap()};

	mCmdList->SetDescriptorHeaps(1, heaps);
	mCmdList->SetGraphicsRootDescriptorTable(1, handle.gpuHandle);

	CBuffer* cameraBuf = SkyBoxBuffer.camera->GetCameraCBuffer();
	if (cameraBuf->updated) {
		mResourceManager.write(cameraBuf->uid, cameraBuf->data.data, cameraBuf->data.size);
	}
	ID3D12Resource* cameraRes = mResourceManager.getResource(cameraBuf->uid);

	mCmdList->SetGraphicsRootConstantBufferView(0,cameraRes->GetGPUVirtualAddress());

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->IASetVertexBuffers(0, 1, &SkyBoxBuffer.boxvbv);
	
	mCmdList->DrawInstanced(36 , 1, 0, 0);
}

bool D3DRenderModule::parseShader(Shader* shader) {
	mPsos[shader->name] = std::make_unique<GraphicPSO>();
	GraphicPSO* pso = mPsos[shader->name].get();

	if (shader->content.GetShaderLanguage() != SHADER_LANGUAGE_HLSL) {
		Log("d3d 12 : invaild shader language the d3d12 api only accepts hlsl as shader language\n");
		mPsos.erase(shader->name);
		return false;
	}

	D3D12_BLEND_DESC blendDesc = {};
	if (!shader->BlendEnable) {
		blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	}
	else {
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;

		D3D12_RENDER_TARGET_BLEND_DESC rtblendDesc = {};
		rtblendDesc.BlendEnable = TRUE;
		rtblendDesc.BlendOp = (D3D12_BLEND_OP)shader->BlendDesc.blendOp;
		rtblendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		rtblendDesc.SrcBlend = (D3D12_BLEND)shader->BlendDesc.blendSrc;
		rtblendDesc.DestBlend = (D3D12_BLEND)shader->BlendDesc.blendDest;
		rtblendDesc.SrcBlendAlpha = (D3D12_BLEND)shader->BlendDesc.blendAlphaSrc;
		rtblendDesc.DestBlendAlpha = (D3D12_BLEND)shader->BlendDesc.blendAlphaDest;
		rtblendDesc.BlendOpAlpha = (D3D12_BLEND_OP)shader->BlendDesc.blendAlphaOp;


		rtblendDesc.LogicOpEnable = FALSE;
		rtblendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;

		for (int i = 0; i != 8; i++) {
			blendDesc.RenderTarget[i] = rtblendDesc;
		}
	}
	pso->SetBlendState(blendDesc);

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = shader->DepthEnable;
	depthStencilDesc.DepthFunc = (D3D12_COMPARISON_FUNC)shader->DepthStencilDesc.depthFunc;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.StencilEnable = shader->StencilEnable;
	
	depthStencilDesc.StencilReadMask = 0xff;
	depthStencilDesc.StencilWriteMask = 0xff;

	D3D12_DEPTH_STENCILOP_DESC depthStencilOp;
	depthStencilOp.StencilDepthFailOp = (D3D12_STENCIL_OP)shader->DepthStencilDesc.stencilDepthFail;
	depthStencilOp.StencilFailOp = (D3D12_STENCIL_OP)shader->DepthStencilDesc.stencilFail;
	depthStencilOp.StencilFunc = (D3D12_COMPARISON_FUNC)shader->DepthStencilDesc.stencilFunc;
	depthStencilOp.StencilPassOp = (D3D12_STENCIL_OP)shader->DepthStencilDesc.stencilPassOp;

	depthStencilDesc.FrontFace = depthStencilOp;
	depthStencilDesc.BackFace = depthStencilOp;

	pso->SetDepthStencilState(depthStencilDesc);

	pso->SetDepthStencilViewFomat(this->mDepthBufferFormat);
	

	CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	D3D12_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.CullMode = (D3D12_CULL_MODE)shader->RasterizerState.cullMode;

	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	rasterizerDesc.FillMode = (D3D12_FILL_MODE)shader->RasterizerState.fillMode;
	rasterizerDesc.ForcedSampleCount = 0;

	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;

	rasterizerDesc.FrontCounterClockwise = FALSE;

	pso->SetRasterizerState(rasterizerDesc);
	pso->SetNodeMask(0);
	pso->SetSampleDesc(1, 0);
	pso->SetSampleMask(0);

	pso->SetPrimitiveTopologyType((D3D12_PRIMITIVE_TOPOLOGY_TYPE)shader->topologyType);

	uint32_t oclOffset = shader->lightEnabled ? 3 : 2;//if the shader need light data,the offset is 3 otherwise there are only two slots

	uint32_t bufferNum = shader->shaderBuffers.size() + oclOffset;
	RootSignature rootSig(bufferNum, 0);//currently we don't support sampling textures
	rootSig[0].initAsConstantBuffer(0, 0);
	rootSig[1].initAsConstantBuffer(1, 0);
	if (shader->lightEnabled) rootSig[2].initAsConstantBuffer(2, 0);

	for (uint32_t i = 0; i != shader->shaderBuffers.size(); i++) {
		if (shader->shaderBuffers[i].type == SHADER_BUFFER_TYPE_CONSTANT) {
			rootSig[i].initAsConstantBuffer(shader->shaderBuffers[i].regID, 0);
		}
	}

	if (!rootSig.EndEditingAndCreate(mDevice.Get())) {
		Log("d3d12 : fail to parse shader %s\n", shader->name.c_str());
		mPsos.erase(shader->name);
		return false;
	}

	pso->SetRootSignature(&rootSig);

	InputLayout layout = {
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	pso->SetInputElementDesc(layout);

	char* shaderData;
	size_t shaderSize;

	if (shader->content.GetShaderContent(SHADER_ENTRY_TYPE_VS, &shaderData, &shaderSize)) {
		ComPtr<ID3DBlob> byteCode = mShaderLoader.CompileFormMemory(shaderData,shaderSize,"VS",SHADER_TYPE::VS);
		if (byteCode == nullptr) {
			Log("d3d12 : fail to parse shader %s\n",shader->name.c_str());
			mPsos.erase(shader->name);
			return false;
		}

		pso->SetVertexShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize());
	}

	if (shader->content.GetShaderContent(SHADER_ENTRY_TYPE_PS, &shaderData, &shaderSize)) {
		ComPtr<ID3DBlob> byteCode = mShaderLoader.CompileFormMemory(shaderData, shaderSize, "PS", SHADER_TYPE::PS);
		if (byteCode == nullptr) {
			Log("d3d12 : fail to parse shader %s\n", shader->name.c_str());
			mPsos.erase(shader->name);
			return false;
		}

		pso->SetPixelShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize());
	}

	pso->SetFlag(D3D12_PIPELINE_STATE_FLAG_NONE);
	pso->SetRenderTargetFormat(mBackBufferFormat);
	
	if (!pso->Create(mDevice.Get())) {
		Log("d3d12 : fail to create pso for shader %s\n",shader->name.c_str());
		mPsos.erase(shader->name);
		return false;
	}

	mRootSignatures[shader->name] = rootSig.GetRootSignature();
	return true;
}

void D3DRenderModule::resize() {
	if (!isInitialized) return;

	if (mFence->GetCompletedValue() < fenceVal) {
		if (FAILED(mFence->SetEventOnCompletion(fenceVal, GpuStallEvent))) {
			Log("d3d12 error : fail to set event on completion system will quit\n");
			dynamic_cast<WindowsApplication*>(app)->Quit();
			return;
		}

		WaitForSingleObject(GpuStallEvent, INFINITE);
	}


	for (int i = 0; i != mBackBufferNum;i++) {
		mBackBuffers[i] = nullptr;
	}

	WindowsApplication* winApp = dynamic_cast<WindowsApplication*>(app);
	Config windowConfig = app->getSysConfig();

	HRESULT hr = mSwapChain->ResizeBuffers(mBackBufferNum,
		windowConfig.width,windowConfig.height,
		mBackBufferFormat,DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(hr)) {
		Log("fail to resize the swapchains\n");
		app->Quit();
		return;
	}

	mCurrentFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i != mBackBufferNum; i++) {
		mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i]));

		mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorIncreament);
	}

	Config sysconfig = winApp->getSysConfig();
	width = sysconfig.width;
	height = sysconfig.height;

	D3D12_RESOURCE_DESC depthDesc;
	depthDesc.Format = mDepthBufferFormat;
	depthDesc.Alignment = 0;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthDesc.Height = height;
	depthDesc.Width = width;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.MipLevels = 1;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;

	mDepthStencilBuffer = nullptr;
	//We assume that the default depth stencil compare function is less so the 
	//default value of the depth buffer is 1.0f
	D3D12_CLEAR_VALUE cValue = {};
	cValue.Format = mDepthBufferFormat;
	cValue.DepthStencil.Depth = 1.f;
	cValue.DepthStencil.Stencil = 0;

	hr = mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&cValue, IID_PPV_ARGS(&mDepthStencilBuffer));

	if (FAILED(hr)) {
		Log("fail to create new depth buffer for resized buffers\n");
		app->Quit();
		return;
	}
	
	mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(),nullptr,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());

	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1.0f;

	sissorRect.bottom = height;
	sissorRect.top = 0;
	sissorRect.left = 0;
	sissorRect.right = width;
}