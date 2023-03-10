#pragma once
#include "pch.h"
#include "DXRAccelerateStructures.h"
#include "Model.h"

namespace DXRRenderer {
	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> scratch; // Scratch memory for AS builder
		ComPtr<ID3D12Resource> result; // Where the AS is
		ComPtr<ID3D12Resource> instanceDesc; // Hold the matrices of the instances
	};
	extern void Init(CommandContext* context);
	extern ComPtr<ID3D12Resource> m_bottomLevelAS; // Storage for the bottom Level AS
	extern void CreateAccelerateStructures(ComPtr<ID3D12Device5> device, ComPtr < ID3D12GraphicsCommandList4> commandList);
	extern void Destroy();

	extern DXRAS::TopLevelAS tlas;
	extern DXRAS::BottomLevelAS blas;
}
