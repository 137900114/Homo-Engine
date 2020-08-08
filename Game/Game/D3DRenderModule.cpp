#include "D3DRenderModule.h"
#include "WindowsApplication.h"
#include "Common.h"
#include <DirectXColors.h>
#include "Timer.h"
#include "InputBuffer.h"

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

	rtvDescriptorIncreament = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
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
	#ifdef _DEBUG || DEBUG
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

	if (!this->mDescriptorAllocator.initiailize(mDevice.Get())) {
		Log("d3d12 error : fail to initialize descriptor handle allocator");
		return false;
	}

	if (!initialize2D()) {
		Log("d3d12 error : fail to initialize 2D render module\n");
		return false;
	}

	
	if (!initializeSingleMesh()) {
		Log("d3d12 error : fail to initialize 3D default render module\n");
		return false;
	}

	if (!initialize2DImage()) {
		Log("d3d12 error : fail to initialize 2D image render module\n");
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

	SingleMesh.gpuLightData = UUID::invalid;
	SingleMesh.gpuCameraData = UUID::invalid;
	SingleMesh.gpuTransform = UUID::invalid;

	Image2DBuffer.currImageVSize = 0;
	Image2DBuffer.imageUploadVertex = UUID::invalid;

	return true;
}

void D3DRenderModule::tick() {
	//------------------------------------//
	if (mFence->GetCompletedValue() < fenceVal) 
		return;

	updateSingleMeshData();

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
	//draw single pass object
	drawSingleMesh();

	for (auto& cmd : drawCommandList) {
		cmd->BindOnCommandList(mCmdList.Get());
		gMemory->Delete(cmd);
	}
	drawCommandList.clear();

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		mBackBuffers[mCurrentFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	));

	mCmdList->Close();

	ID3D12CommandList* toExcute[] = {mCmdList.Get()};
	mCmdQueue->ExecuteCommandLists(_countof(toExcute),toExcute);

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mBackBufferNum;
	mCmdQueue->Signal(mFence.Get(), ++fenceVal);

	if (FAILED(mSwapChain->Present(0, 0))) {
		Log("fail to present swap chain\n");
		dynamic_cast<WindowsApplication*>(app)->Quit();
		return;
	}
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

void D3DRenderModule::bindScene(Scene * scene) {
	Data3D.currScene = scene;
}

void D3DRenderModule::releaseScene(Scene& scene) {
	for (auto& item : scene.meshs) {
		mResourceManager.releaseResource(item.second.gpuDataToken.index);
		mResourceManager.releaseResource(item.second.gpuDataToken.vertex);
	}

	for (auto& item : scene.images) {
		mResourceManager.releaseResource(item.second.gpuDataToken);
	}

	for (auto& item : scene.buffers) {
		mResourceManager.releaseResource(item.second.gpuDataToken);
	}
}

//currently we don't consider custom material
void D3DRenderModule::drawSingleMesh(Mesh& mesh,SceneMaterial* mat) {
	initializeSingleMeshData();
	SingleMesh.mesh = &mesh;
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

	for (auto& item : scene.images) {
		if (item.second.gpuDataToken == UUID::invalid) {
			item.second.gpuDataToken =
				mResourceManager.uploadTexture(item.second, D3D12_RESOURCE_STATE_COMMON);
		}
	}

	for (auto& item : scene.buffers) {
		//constant buffers need to align to 256
		if (item.second.gpuDataToken == UUID::invalid) {
			item.second.gpuDataToken =
				mResourceManager.uploadStaticBuffer(item.second.buffer,
					D3D12_RESOURCE_STATE_COMMON,
					true);
		}
	}

}

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
		SingleMesh.lightu += 0.005 * PI;
	else if (gInput.KeyHold(InputBuffer::D))
		SingleMesh.lightu -= 0.005 * PI;

	SingleMesh.lightData.Light[0].lightDirection =
		Vector3(sin(SingleMesh.lightu), 0., cos(SingleMesh.lightu));

	Mat4x4 rotate = MatrixRotateY( PI * 0.1 * gTimer.TotalTime());//(reinterpret_cast<float*>(&mat));

	Mat4x4 world = MatrixPosition(Vector3(0, 0, SingleMesh.dis));
	world = mul(world, rotate);
	SingleMesh.objectData.world = world.T();
	SingleMesh.objectData.transInvWorld = world.R();
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

	Image2DBuffer.images.push_back(srvHandle.gpuHandle);

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

	for (int i = 0; i != Image2DBuffer.images.size(); i++) {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mVertResource->GetGPUVirtualAddress() + sizeof(ImageVertex2D) * i * 6;
		vbv.SizeInBytes = sizeof(ImageVertex2D) * 6;
		vbv.StrideInBytes = sizeof(ImageVertex2D);

		D3DDrawContext* draw = gMemory->New<D3DDrawContext>(mPsos["Image2D"]->GetPSO(), mRootSignatures["Image2D"].Get(),
			mDescriptorAllocator.getDescriptorHeap(),2, vbv, 6);
		
		draw->SetShaderParameter(1, ShaderParameter(ProjResource->GetGPUVirtualAddress(),
			D3D12_ROOT_PARAMETER_TYPE_CBV, 1));
		draw->SetShaderParameter(0, ShaderParameter(Image2DBuffer.images[i],
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, 0));

		drawCommandList.push_back(draw);
	}

	Image2DBuffer.images.clear();
	Image2DBuffer.imageVertexList.clear();
}