
#pragma comment(lib, "d3d11.lib")

#include <Detours.h>

#include <d3d11.h>

ID3D11Device*        g_Device;
ID3D11DeviceContext* g_DeviceContext;
IDXGISwapChain*      g_SwapChain;

std::atomic<bool>                             setSampler = false;
ID3D11SamplerState* m_anisoSampleState;


ID3D11Buffer* g_textureDims = NULL;

// Define the constant data used to communicate with shaders.
struct PS_CONSTANT_BUFFER_DIMS
{
	float      width;
	float      height;
};


decltype(&ID3D11DeviceContext::PSSetSamplers) ptrPSSetSamplers;

void WINAPI hk_PSSetSamplers(ID3D11DeviceContext* This,
	UINT                                   StartSlot,
	UINT                                   NumSamplers,
	ID3D11SamplerState* const* ppSamplers)
{
	if (setSampler) 
	{
		if (StartSlot == 3 && NumSamplers == 1) {
			g_DeviceContext->PSSetConstantBuffers(0, 3, &g_textureDims);
			setSampler = false;
		}
	}
	(This->*ptrPSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}


void CreateTextureDimsBuffer()
{
	// Set initial data
	PS_CONSTANT_BUFFER_DIMS init = { 1024, 1024 };

	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(PS_CONSTANT_BUFFER_DIMS);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &init;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the buffer.
	g_Device->CreateBuffer(&cbDesc, &InitData, &g_textureDims);
}
	
//struct Hooks
//	{
//	struct BSLightingShader_SetupMaterial
//	{
//		static void thunk(struct BSLightingShader* This, struct RenderPass* pass)
//		{
//			setSampler = true;
//			func(This, pass);
//		}
//
//		static inline REL::Relocation<decltype(thunk)> func;
//	};
//
//	struct BSGraphics_Renderer_Init_InitD3D
//	{
//		static void thunk()
//		{
//			logger::info("Calling original Init3D");
//			func();
//			logger::info("Accessing render device information");
//			auto manager = RE::BSRenderManager::GetSingleton();
//			g_Device = manager->GetRuntimeData().forwarder;
//			g_DeviceContext = manager->GetRuntimeData().context;
//			g_SwapChain = manager->GetRuntimeData().swapChain;
//			logger::info("Detouring virtual function tables");
//			*(uintptr_t*)&ptrPSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_PSSetSamplers, 10);
//			logger::info("Setting sleep mode for the first time");
//
//
//
//
//		}
//		static inline REL::Relocation<decltype(thunk)> func;
//	};
//
//	#define PatchIAT(detour, module, procname) Detours::IATHook(g_ModuleBase, (module), (procname), (uintptr_t)(detour));
//
//	static void Install()
//	{
//		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
//		stl::write_vfunc<0x4, BSLightingShader_SetupMaterial>(RE::VTABLE_BSLightingShader[0]);
//		logger::info("Installed render hooks");
//	}
//};

//void PatchD3D11()
//{
//	Hooks::Install();
//}
