#include "FormUtil.h"


auto FormUtil::GetIdentifierFromForm(const RE::TESForm* a_form) -> std::string
{
	auto editorID = a_form->GetFormEditorID();
	if (editorID && strlen(editorID) > 1) {
		return std::format("{}", editorID);
	}
	if (auto file = a_form->GetFile()) {
		return std::format("{:X}|{}", a_form->GetLocalFormID(), file->GetFilename());
	}
	return std::format("{:X}|Generated", a_form->GetLocalFormID());
}
