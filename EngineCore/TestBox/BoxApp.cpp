//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "..//EngineCore/RootSignature.h"
#include "..//EngineCore/PipelineStateObject.h"
#include "..//EngineCore/DynamicDescriptorHeap.h"
#include "..//EngineCore/Device.h"
#include "../EngineCore/GPUBuffer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Core;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

void* ptr;

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:

	std::unique_ptr<RootSignature> mRootSignature;

	UploadBuffer upload;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::unique_ptr<GraphicPSO> mPSO;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	GPUBuffer boxVert;
	ComPtr<ID3D12Resource> boxVertUpload;
	GPUBuffer boxIndex;
	ComPtr<ID3D12Resource> boxIndexUpload;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};

std::unique_ptr<BoxApp> app = nullptr;

ID3D12Device* Core::Device::GetCurrentDevice() {
	return app->md3dDevice.Get();
}

UINT Core::Device::GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE type) {
	return app->md3dDevice->GetDescriptorHandleIncrementSize(type);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		app = std::make_unique<BoxApp>(hInstance);
		if (!app->Initialize())
			return 0;

		return app->Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

BoxApp::BoxApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	boxVertUpload = nullptr;
	boxIndexUpload = nullptr;

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

	upload.Upload2Buffer(&objConstants, 1, 0);
}

void BoxApp::Draw(const GameTimer& gt)
{
	GraphicCommandBuffer& buffer = CommandBuffer::Start().GetGraphicBuffer();
	buffer.SetViewPortAndScissorRect(mScreenViewport,mScissorRect);
	buffer.TransitionResource(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	float f[] = { .4,1.,1.,1. };
	buffer.ClearDepthAndStencilBuffer(DepthStencilView(), 1, 0);
	buffer.ClearRenderTargetBuffer(CurrentBackBufferView(), f);
	
	buffer.SetRenderTargetView(1, &CurrentBackBufferView(), DepthStencilView());

	buffer.SetRootSignature(mRootSignature.get());
	buffer.SetGraphicPipelineState(mPSO.get());

	buffer.SetConstantBufferView(0, upload.GetCBV());

	buffer.SetVertexBuffer(boxVert.GetVertexBufferView());
	buffer.SetIndexBuffer(boxIndex.GetIndexBufferView());
	buffer.SetPrimitiveTopoloy(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	buffer.DrawIndexed(boxIndex.GetElementNum(), 0, 0);

	buffer.TransitionResource(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
	buffer.End();

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildConstantBuffers()
{
	upload.Create(d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants)), 1);
}


void BoxApp::BuildRootSignature()
{
	mRootSignature = std::make_unique<RootSignature>(1,0);
	(*mRootSignature)[0].initAsConstantBuffer(0,0);
	mRootSignature->EndEditingAndCreate();
}

void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"..\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"..\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }),
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	boxVert.Create(sizeof(Vertex), vertices.size(),vertices.data());
	boxIndex.Create(sizeof(uint16_t),indices.size(),indices.data());
}

void BoxApp::BuildPSO()
{
	mPSO = std::make_unique<GraphicPSO>();
	mPSO->SetBDRDefault();
	mPSO->SetInputElementDesc(mInputLayout);
	mPSO->SetRootSignature(mRootSignature.get());
	mPSO->SetVertexShader(mvsByteCode->GetBufferPointer(), mvsByteCode->GetBufferSize());
	mPSO->SetPixelShader(mpsByteCode->GetBufferPointer(),mpsByteCode->GetBufferSize());
	mPSO->SetSampleMask(UINT_MAX);
	mPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	mPSO->SetRenderTargetFormat(mBackBufferFormat);
	mPSO->SetDepthStencilViewFomat(mDepthStencilFormat);
	mPSO->SetSampleDesc(1,0);

	mPSO->Create();
}