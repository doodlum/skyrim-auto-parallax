
auto UpdateMaterialParallax(RE::NiAVObject* a_node)
{
	using namespace RE;
	BSVisit::TraverseScenegraphGeometries(a_node, [&](BSGeometry* a_geometry) -> BSVisit::BSVisitControl {
		using State = BSGeometry::States;
		using Feature = BSShaderMaterial::Feature;
		if (auto effect = a_geometry->GetGeometryRuntimeData().properties[State::kEffect].get()) {
			if (auto lightingShader = netimmerse_cast<BSLightingShaderProperty*>(effect)) {
				const auto material = static_cast<BSLightingShaderMaterialBase*>(lightingShader->material);
				if (material->GetFeature() == BSShaderMaterial::Feature::kParallax) {
					// ProjectedUV is the conflict.
					if (!lightingShader->flags.any(BSShaderProperty::EShaderPropertyFlag::kProjectedUV) && material->textureSet) {
						auto parallax = static_cast<BSLightingShaderMaterialParallax*>(lightingShader->material);
						// Only enable if the parallax file handle exists
						lightingShader->SetFlags(BSShaderProperty::EShaderPropertyFlag8::kParallax, parallax->heightTexture && parallax->heightTexture->unk40);			
					} else {
						lightingShader->SetFlags(BSShaderProperty::EShaderPropertyFlag8::kParallax, false);
					}
				}
			} 
		}
		return BSVisit::BSVisitControl::kContinue;
	});
}

struct Hooks
{
	struct Clone3D
	{
		static RE::NiAVObject* thunk(RE::TESBoundObject* a_base, RE::TESObjectREFR* a_ref, bool a_arg3)
		{
			auto ret = func(a_base, a_ref, a_arg3);
			UpdateMaterialParallax(ret);
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
	Hooks::Install();
}

