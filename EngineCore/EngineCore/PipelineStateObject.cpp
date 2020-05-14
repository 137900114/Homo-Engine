#include "PipelineStateObject.h"
#include "trivial.h"
#include "Device.h"

using namespace Core;

bool GraphicPSO::Create() {
	if (!m_PSODesc.pRootSignature) {
		DEBUG_OUTPUT("GraphicPSO::Create : rootsignature can't be a null when creating\n");
		return false;
	}
	if (m_InputLayouts.empty()) {
		DEBUG_OUTPUT("GraphicPSO::Create : can't create pso with empty input layout");
		return false;
	}

	D3D12_INPUT_LAYOUT_DESC layout;
	layout.NumElements = m_InputLayouts.size();
	layout.pInputElementDescs = m_InputLayouts.data();

	m_PSODesc.InputLayout = layout;
	HRESULT hr = Device::GetCurrentDevice()->CreateGraphicsPipelineState(
		&m_PSODesc, IID_PPV_ARGS(&m_Pso)
	);
	if (FAILED(hr)) {
		DEBUG_OUTPUT("GraphicPSO::Create :: fail to create pipeline state object");
		return false;
	}

	return true;
}

void GraphicPSO::SetRootSignature(RootSignature* rootSig) {
	if (!rootSig->RootIsCreated()) {
		DEBUG_OUTPUT("GraphicPSO::SetRootSignature : set rootsignature before it has been created ");
		return;
	}
	m_PSODesc.pRootSignature = rootSig->GetRootSignature();
}


void GraphicPSO::SetBDRDefault() {
	m_PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	m_PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	m_PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
}

void GraphicPSO::SetRenderTargetFormat(UINT num,const DXGI_FORMAT* formats) {
	ASSERT_WARNING(num == 0 || formats == nullptr ,"GraphicPSO::SetRenderTargetFormat the num of render target can't be 0");
	for (int i = 0; i != num; i++) {
		m_PSODesc.RTVFormats[i] = formats[i];
	}
	for (int i = num; i != _countof(m_PSODesc.RTVFormats); i++) {
		m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	m_PSODesc.NumRenderTargets = num;
}

ID3D12PipelineState* GraphicPSO::GetPSO() {
	ASSERT_WARNING(m_Pso.Get() == nullptr,"you must create the pso before you can get it");
	return m_Pso.Get();
}

void ComputePSO::SetRootSignature(RootSignature* rootSig) {
	if (rootSig->RootIsCreated()) {
		DEBUG_OUTPUT("ComputePSO::SetRootSignature : set rootsignature before it has been created");
		return;
	}
	m_RootSig = rootSig->GetRootSignature();
}

bool ComputePSO::Create() {
	if (!m_RootSig) {
		DEBUG_OUTPUT("ComputePSO::Create : root signature should not be null when creating");
		return false;
	}

	m_PSODesc.pRootSignature = m_RootSig;

	HRESULT hr = Device::GetCurrentDevice()->CreateComputePipelineState(
		&m_PSODesc, IID_PPV_ARGS(&m_Pso));
	if (FAILED(hr)) {
		DEBUG_OUTPUT("ComputePSO::Create : fail to create compute pipeline state");
		return false;
	}
	return true;
}