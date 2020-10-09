#include "D3DDrawContext.h"
#include "Memory.h"

namespace Game {
	extern MemoryModule gMemory;
}

Game::D3DDrawContext::D3DDrawContext(ID3D12PipelineState* pso, ID3D12RootSignature* rootSig,
	ID3D12DescriptorHeap* descHeap,uint32_t parameterNum, D3D12_VERTEX_BUFFER_VIEW vbv, uint32_t vertNum, uint32_t vertBaseIndex,
	D3D_PRIMITIVE_TOPOLOGY topology, uint32_t instanceNum):
	parameterNum(parameterNum),vbv(vbv),vertNum(vertNum),vertexBaseIndex(vertBaseIndex),
	isIndexed(false),topology(topology),drawInstanced(instanceNum > 1),instanceNum(instanceNum)
{
	parameters = gMemory.NewArray<D3DShaderParameter>(parameterNum);
	this->pso = pso;
	this->rootSig = rootSig;
	this->descriptorHeap = descHeap;
}


Game::D3DDrawContext::D3DDrawContext(ID3D12PipelineState* pso, ID3D12RootSignature* rootSig,
	ID3D12DescriptorHeap* descHeap,uint32_t parameterNum, D3D12_VERTEX_BUFFER_VIEW vbv, uint32_t vertNum,
	D3D12_INDEX_BUFFER_VIEW ibv, uint32_t indexNum, uint32_t vertBaseIndex,
	uint32_t indexBaseIndex , D3D_PRIMITIVE_TOPOLOGY topology,
	uint32_t instanceNum ) :pso(pso), rootSig(rootSig),
	parameterNum(parameterNum), vbv(vbv), vertNum(vertNum),ibv(ibv),indexNum(indexNum),
	vertexBaseIndex(vertBaseIndex), indexBaseIndex(indexBaseIndex) ,isIndexed(true), topology(topology), 
	drawInstanced(instanceNum > 1), instanceNum(instanceNum),
	descriptorHeap(descHeap){

	parameters = gMemory.NewArray<D3DShaderParameter>(parameterNum);
	this->pso = pso;
	this->rootSig = rootSig;
	this->descriptorHeap = descHeap;
}



void Game::D3DDrawContext::BindOnCommandList(ID3D12GraphicsCommandList* cmdList) {
	
	cmdList->SetPipelineState(pso.Get());
	cmdList->SetGraphicsRootSignature(rootSig.Get());

	ID3D12DescriptorHeap* HeapList[] = { descriptorHeap.Get()};
	cmdList->SetDescriptorHeaps(1, HeapList);

	for (int i = 0; i != parameterNum; i++) {
		switch (parameters[i].type) {
		case D3D12_ROOT_PARAMETER_TYPE_CBV:
			cmdList->SetGraphicsRootConstantBufferView(parameters[i].parameterIndex,
				parameters[i].Data.address);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_SRV:
			cmdList->SetGraphicsRootShaderResourceView(parameters[i].parameterIndex,
				parameters[i].Data.address);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_UAV:
			cmdList->SetGraphicsRootUnorderedAccessView(parameters[i].parameterIndex,
				parameters[i].Data.address);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			cmdList->SetGraphicsRootDescriptorTable(parameters[i].parameterIndex,
				parameters[i].Data.handle);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			cmdList->SetGraphicsRoot32BitConstants(parameters[i].parameterIndex,
				parameters[i].Data.constant32Bit.size,
				parameters[i].Data.constant32Bit.data,0);
			break;
		default:
			Log("d3d12 : invalid parameter type while binding parameters\n");
			break;
		}
	}
	
	cmdList->IASetPrimitiveTopology(topology);
	cmdList->IASetVertexBuffers(0, 1, &vbv);

	if (!drawInstanced) instanceNum = 1;

	if (isIndexed) {
		cmdList->IASetIndexBuffer(&ibv);
		cmdList->DrawIndexedInstanced(indexNum, instanceNum, indexBaseIndex, vertexBaseIndex, 0);
	}
	else {
		cmdList->DrawInstanced(vertNum, instanceNum, vertexBaseIndex, 0);
	}
	
}


Game::D3DDrawContext::~D3DDrawContext() {
	//release the temporary allocated memory
	gMemory.DeleteArray(parameterNum, parameters);
	this->pso = nullptr;
	this->rootSig = nullptr;
	this->descriptorHeap = nullptr;
}