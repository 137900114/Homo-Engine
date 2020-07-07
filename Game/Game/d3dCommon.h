#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "d3d12x.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;