#include <Detours.h>

//	LoadTexture(path, 1, newTexture, false);
static void LoadTexture(const char* path, uint8_t unk1, RE::NiPointer<RE::NiTexture>& texture, bool unk2)
{
	using func_t = decltype(&LoadTexture);
	REL::Relocation<func_t> func{ REL::RelocationID(26570, 27203) };
	func(path, unk1, texture, unk2);
}

//Source, 0, &newTexture, 0, 0, 0);
//RE::NiSourceTexture* BSShaderManager_GetTexture(char* a_source, bool a_background, RE::NiSourceTexture& a_texture, bool unk1 = false, bool unk2 = false, bool unk3 = false)
//{
//	using func_t = decltype(&BSShaderManager_GetTexture);
//	REL::Relocation<func_t> func{ REL::RelocationID(98986, 105640) };
//	func(a_source, a_background, a_texture, unk1, unk2, unk3);
//}

void InvalidateTextures(RE::BSLightingShaderProperty* a_shader, uint32_t a_unk1)
{
	using func_t = decltype(&InvalidateTextures);
	REL::Relocation<func_t> func{ REL::RelocationID(99865, 99865) };
	func(a_shader, a_unk1);
}

uint32_t InitializeShader(RE::BSLightingShaderProperty* a_shader, RE::BSGeometry* a_geometry)
{
	using func_t = decltype(&InitializeShader);
	REL::Relocation<func_t> func{ REL::RelocationID(99862, 99862) };
	return func(a_shader, a_geometry);
}

void PatchD3D11();


RE::NiTexturePtr* GetHeightTexture(RE::BSLightingShaderMaterialParallax* a_material)
{
	return (RE::NiTexturePtr*)&a_material->heightTexture;
}

auto UpdateMaterialParallax(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_node)
{
	using namespace RE;
	BSVisit::TraverseScenegraphGeometries(a_node, [&](BSGeometry* a_geometry) -> BSVisit::BSVisitControl {
		using State = BSGeometry::States;
		using Feature = BSShaderMaterial::Feature;
		auto effect = a_geometry->GetGeometryRuntimeData().properties[State::kEffect].get();
		if (effect) {
			auto lightingShader = netimmerse_cast<BSLightingShaderProperty*>(effect);
			if (lightingShader) {
				auto material = static_cast<BSLightingShaderMaterialBase*>(lightingShader->material);
				if (material->GetFeature() == BSShaderMaterial::Feature::kDefault) {
					if (material->textureSet) {
						std::string diffuse = material->textureSet->GetTexturePath(BSTextureSet::Texture::kDiffuse);
						//logger::info("Before rename {}", diffuse);
						if (diffuse.rfind("_d.dds") == diffuse.length() - 6) {
							diffuse = diffuse.substr(0, diffuse.length() - 6);
						} else if (diffuse.rfind(".dds") == diffuse.length() - 4) {
							diffuse = diffuse.substr(0, diffuse.length() - 4);
						}
						std::string parallax = std::format("{}_p.dds", diffuse);
						//logger::info("After rename {}", parallax);
						bool good = false;
						{
							BSResourceNiBinaryStream a_fileStream{ parallax };
							good = a_fileStream.good();
						}
						if (good) {
							good = true;
							logger::warn("Found parallax at {}", parallax);
							BSLightingShaderMaterialParallax* parallaxMaterial = BSLightingShaderMaterialBase::CreateMaterial<BSLightingShaderMaterialParallax>();
							parallaxMaterial->CopyBaseMembers(material);
							NiPointer<BSShaderTextureSet> niTextureSet{ BSShaderTextureSet::Create() };
							for (uint32_t i = 0; i < BSTextureSet::Textures::kUsedTotal; i++) {
								niTextureSet->SetTexturePath((BSTextureSet::Texture)i, material->textureSet->GetTexturePath((BSTextureSet::Texture)i));
							}
							niTextureSet->SetTexturePath(BSTextureSet::Texture::kHeight, parallax.c_str());
							lightingShader->SetFlags(BSShaderProperty::EShaderPropertyFlag8::kParallax, true);
							lightingShader->SetMaterial(parallaxMaterial, true);
							((BSLightingShaderMaterialParallax*)lightingShader->material)->ClearTextures();
							((BSLightingShaderMaterialParallax*)lightingShader->material)->SetTextureSet(niTextureSet);
							InvalidateTextures(lightingShader, 0);
							parallaxMaterial->~BSLightingShaderMaterialParallax();				
							InitializeShader(lightingShader, a_geometry);

						} else {
							//logger::info("No parallax at {}", parallax);
						}

					} else {
						logger::warn("No texture set {:X}", a_ref->GetFormID());
					}
				}
			}
		}

		return BSVisit::BSVisitControl::kContinue;
	});
}


RE::NiAVObject* hk_Load3D(RE::TESObjectREFR* a_ref, bool a_backgroundLoading);

decltype(&hk_Load3D) ptrLoad3D;

RE::NiAVObject* hk_Load3D(RE::TESObjectREFR* a_ref, bool a_backgroundLoading) 
{
	auto ret = (ptrLoad3D)(a_ref, a_backgroundLoading);
	UpdateMaterialParallax(a_ref, ret);
	return ret;
}

struct Hooks
{
	struct TESObjectREFR_Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			auto ret = func(a_ref, a_backgroundLoading);
			UpdateMaterialParallax(a_ref, ret);
			return ret;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};


	static void Install()
	{
	//	*(uintptr_t*)&ptrLoad3D = Detours::X64::DetourFunction(REL::RelocationID(19300, 19300).address(), (uintptr_t)&hk_Load3D);
	//	*(uintptr_t*)&ptrLoad3D = Detours::X64::DetourFunction((uintptr_t)GetModuleHandleA(nullptr) + 0x28FB90, (uintptr_t)&hk_Load3D);

		stl::write_vfunc<RE::TESObjectREFR, 0x6A, TESObjectREFR_Load3D>();
	}
};

void Load()
{
	Hooks::Install();
	//SKSE::AllocTrampoline(1 << 6);
	//BSShaderHooks::Install();
	//PatchD3D11();
}

/*
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */

//#include "crc32_hash.hpp"
//#include <filesystem>
//#include <fstream>
//#include <ReShade/reshade.hpp>
//
//using namespace reshade::api;
//
//static void dump_shader_code([[maybe_unused]] device_api device_type, const shader_desc& desc, const wchar_t* extension)
//{
//	if (desc.code_size == 0)
//		return;
//
//	uint32_t shader_hash = compute_crc32(static_cast<const uint8_t*>(desc.code), desc.code_size);
//
//	// Prepend executable file name to image files
//	wchar_t file_prefix[MAX_PATH] = L"";
//	GetModuleFileNameW(nullptr, file_prefix, ARRAYSIZE(file_prefix));
//
//	wchar_t hash_string[11];
//	swprintf_s(hash_string, L"0x%08X", shader_hash);
//
//	std::filesystem::path dump_path = L"Shaders\\";
//	//dump_path += file_prefix;
//	//dump_path += L'_';
//	//dump_path += L"shader_";
//	dump_path += hash_string;
//	dump_path += extension;
//	logger::info("Dumping shader at {}", dump_path.string());
//
//	std::ofstream file(dump_path, std::ios::binary);
//	file.write(static_cast<const char*>(desc.code), desc.code_size);
//}
//
//static bool on_create_pipeline(device* device, pipeline_layout, uint32_t subobject_count, const pipeline_subobject* subobjects)
//{
//	const device_api device_type = device->get_api();
//
//	if (!std::filesystem::exists("Shaders\\"))
//		std::filesystem::create_directories("Shaders\\");
//
//	// Go through all shader stages that are in this pipeline and dump the associated shader code
//	for (uint32_t i = 0; i < subobject_count; ++i) {
//		switch (subobjects[i].type) {
//		case pipeline_subobject_type::vertex_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".vso");
//			break;
//		case pipeline_subobject_type::hull_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".hso");
//			break;
//		case pipeline_subobject_type::domain_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".dso");
//			break;
//		case pipeline_subobject_type::geometry_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".gso");
//			break;
//		case pipeline_subobject_type::pixel_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".pso");
//			break;
//		case pipeline_subobject_type::compute_shader:
//			dump_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data), L".cso");
//			break;
//		}
//	}
//
//	return false;
//}
//
//extern "C" __declspec(dllexport) const char* NAME = "Shader Dump";
//extern "C" __declspec(dllexport) const char* DESCRIPTION = "Example add-on that dumps all shader binaries used by the application to disk.";
//
//BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
//{
//	switch (fdwReason) {
//	case DLL_PROCESS_ATTACH:
//		if (!reshade::register_addon(hModule))
//			return FALSE;
//		reshade::register_event<reshade::addon_event::create_pipeline>(on_create_pipeline);
//		break;
//	case DLL_PROCESS_DETACH:
//		reshade::unregister_addon(hModule);
//		break;
//	}
//
//	return TRUE;
//}
