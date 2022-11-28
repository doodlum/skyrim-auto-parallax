
#include <SimpleIni.h>
#include "FormUtil.h"

#define GetSettingBool(a_section, a_setting) a_setting = ini.GetBoolValue(a_section, #a_setting, false);

bool bAutoEnableParallax;
bool bEnableDebugLog;
bool bEnableDebugTextures;

void LoadINI()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\AutoParallax.ini");

	GetSettingBool("Experimental", bAutoEnableParallax);
	GetSettingBool("Debug", bEnableDebugLog);
	GetSettingBool("Debug", bEnableDebugTextures);
}

void BSShaderManager_GetTexture(const char* a_source, bool a_background, RE::NiPointer<RE::NiSourceTexture>& a_texture, bool unk1 = false, bool unk2 = false, bool unk3 = false)
{
	using func_t = decltype(&BSShaderManager_GetTexture);
	REL::Relocation<func_t> func{ REL::RelocationID(98986, 105640) };
	func(a_source, a_background, a_texture, unk1, unk2, unk3);
}

bool BSFileExists(std::string a_path)
{
	RE::BSResourceNiBinaryStream fileStream{ a_path };
	return fileStream.good();
}

std::string Parallaxilize(const char* a_path, std::string a_suffix)
{
	if (a_path) {
		std::string path = a_path;
		if (path.rfind(a_suffix + ".dds") == path.length() - 6) {
			return path.substr(0, path.length() - 6) + "_p.dds";
		} else if (path.rfind(".dds") == path.length() - 4) {
			return path.substr(0, path.length() - 4) + "_p.dds";
		}
	}
	return "";
}

bool FindParallax(RE::BSTextureSet* a_tex, std::string& o_path)
{
	// Some meshes could have parallax textures but no parallax flag
	auto height = a_tex->GetTexturePath(RE::BSTextureSet::Texture::kHeight);
	if (height && BSFileExists(height)) {
		o_path = height;
		return true;
	}

	// Diffuse goes first, more likely to be unique
	std::string contender = Parallaxilize(a_tex->GetTexturePath(RE::BSTextureSet::Texture::kDiffuse), "_d");
	if (BSFileExists(contender)) {
		o_path = contender;
		return true;
	}

	// Normal maps as a backup, shared between more objects
	contender = Parallaxilize(a_tex->GetTexturePath(RE::BSTextureSet::Texture::kNormal), "_n");
	if (BSFileExists(contender)) {
		o_path = contender;
		return true;
	}
	return false;
}

bool HasNiAlphaProperty(RE::BSGeometry* a_geometry)
{
	return a_geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::State::kProperty].get();
}
//
//uint8_t* ResizeVerts(const uint8_t* in, uint32_t count, uint32_t byteSize)
//{
//	byteSize /= sizeof(uint8_t);
//	uint32_t newByteSize = byteSize + 4;
//	uint8_t* out = new uint8_t[count * newByteSize];  // num verts * size + vertex color
//	for (uint32_t i = 0; i < count; i++) { // for each vert
//		uint32_t base = i * newByteSize;
//		for (uint32_t k = 0; k < byteSize; k++) { // for each original byte
//			out[base + k] = in[(i * byteSize) + k];  // vert offset * new size + byte offset
//		}
//		for (uint32_t j = 0; j < 4; j++) { // for each new byte
//			out[base + (byteSize - 1) + j] = (uint8_t)1.0f;  // set to 1.0f
//		}
//	}
//	return out;
//}

auto UpdateMaterialParallax(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_node)
{
	RE::BSVisit::TraverseScenegraphGeometries(a_node, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
		if (auto effect = a_geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::State::kEffect].get()) {
			if (auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect)) {
				const auto material = static_cast<RE::BSLightingShaderMaterialBase*>(lightingShader->material);
				auto       diffuse = material->textureSet.get()->GetTexturePath(RE::BSTextureSet::Textures::kDiffuse);
				if (diffuse) {
					std::string dstring = diffuse;
					if (dstring.contains("TundraMossTEST01")) {
						logger::info("START");
						for (int i = 0; i < 64; i++) {
							static constexpr auto                     BIT64 = static_cast<std::uint64_t>(1);
							RE::BSShaderProperty::EShaderPropertyFlag flag = (RE::BSShaderProperty::EShaderPropertyFlag)(BIT64 << i);
							if (lightingShader->flags.any(flag)) {
								logger::info("flags: {}", i);
							}
						}
					}
				}
				if (material->GetFeature() == RE::BSShaderMaterial::Feature::kDefault) {
					if (bAutoEnableParallax && !lightingShader->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kProjectedUV, RE::BSShaderProperty::EShaderPropertyFlag::kSkinned, RE::BSShaderProperty::EShaderPropertyFlag::kLODObjects, RE::BSShaderProperty::EShaderPropertyFlag::kTreeAnim) && lightingShader->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kVertexColors) && !HasNiAlphaProperty(a_geometry)) {
						// It doesn't hurt to check
						if (material->textureSet) {
							std::string parallax;
							// See if any parallax file exists
							if (FindParallax(material->textureSet.get(), parallax)) {
								// Create a new material
								if (auto newMaterial = RE::BSLightingShaderMaterialParallax::CreateMaterial<RE::BSLightingShaderMaterialParallax>(); newMaterial) {
									newMaterial->CopyBaseMembers(material);

									RE::NiPointer<RE::NiSourceTexture> tex = RE::NiPointer<RE::NiSourceTexture>();
									// Arguments were determined by tracking original function calls for parallax textures
									BSShaderManager_GetTexture(parallax.c_str(), 1, tex, 0, 1, 1);
									newMaterial->heightTexture = tex;

									if (bEnableDebugLog) {
										logger::info("{}", parallax);
									}

									if (bEnableDebugTextures) {
										newMaterial->ClearTextures();
									}
									
									lightingShader->SetMaterial(newMaterial, true);

									lightingShader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kParallax, true);
									lightingShader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kBackLighting, false);

									//auto count = a_geometry->AsTriShape()->GetTrishapeRuntimeData().vertexCount;
									//auto size = a_geometry->GetGeometryRuntimeData().rendererData->vertexDesc.GetSize();
									//auto data = a_geometry->GetGeometryRuntimeData().rendererData->rawVertexData;
									//a_geometry->GetGeometryRuntimeData().rendererData->rawVertexData = ResizeVerts(data, count, size);
									//a_geometry->GetGeometryRuntimeData().rendererData->vertexDesc.SetFlag(RE::BSGraphics::Vertex::Flags::VF_COLORS);
									//a_geometry->GetGeometryRuntimeData().vertexDesc.SetFlag(RE::BSGraphics::Vertex::Flags::VF_COLORS);
									//lightingShader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kVertexColors, true);

									// Reinitialise some geometry data
									lightingShader->SetupGeometry(a_geometry);
									lightingShader->FinishSetupGeometry(a_geometry);

									// Discard the new material which is no longer needed
									newMaterial->~BSLightingShaderMaterialParallax();
									RE::free(newMaterial);
								}
							}
						}
					}
				} else if (material->GetFeature() == RE::BSShaderMaterial::Feature::kParallax) {
					// ProjectedUV is the conflict.
					if (!lightingShader->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kProjectedUV) && material->textureSet) {
						auto parallax = static_cast<RE::BSLightingShaderMaterialParallax*>(lightingShader->material);
						// Only enable if the parallax file handle exists
						if (parallax->heightTexture && parallax->heightTexture->unk40) 
						{
							if (bEnableDebugLog) {
								logger::info("Disabled {}", FormUtil::GetIdentifierFromForm(a_ref));
							}
							if (bEnableDebugTextures) {
								material->ClearTextures();
							}
							
							lightingShader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kParallax, false);
						}
					} else {
						if (bEnableDebugLog)
							logger::info("Disabled {}", FormUtil::GetIdentifierFromForm(a_ref));

						if (bEnableDebugTextures)
							material->ClearTextures();
						lightingShader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kParallax, false);
					}
				}
			}
		}
		return RE::BSVisit::BSVisitControl::kContinue;
	});
}

struct Hooks
{
	struct Clone3D
	{
		static RE::NiAVObject* thunk(RE::TESBoundObject* a_base, RE::TESObjectREFR* a_ref, bool a_arg3)
		{
			auto ret = func(a_base, a_ref, a_arg3);
			UpdateMaterialParallax(a_ref, ret);
			return ret;
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            size = 0x40;
	};

	static void Install()
	{
		stl::write_vfunc<RE::TESObjectSTAT, Clone3D>();
		stl::write_vfunc<RE::BGSMovableStatic, 2, Clone3D>();
		stl::write_vfunc<RE::TESObjectCONT, Clone3D>();
	}
};

void Load()
{
	logger::info("Installing hooks");
	Hooks::Install();
	logger::info("Loading INI");
	LoadINI();
}
