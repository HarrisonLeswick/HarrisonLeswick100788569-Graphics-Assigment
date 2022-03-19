#include "DebugWindow.h"
#include "Application/Application.h"
#include "Application/ApplicationLayer.h"
#include "Application/Layers/RenderLayer.h"

DebugWindow::DebugWindow() :
	IEditorWindow()
{
	Name = "Debug";
	SplitDirection = ImGuiDir_::ImGuiDir_None;
	SplitDepth = 0.5f;
	Requirements = EditorWindowRequirements::Menubar;
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::RenderMenuBar() 
{
	Application& app = Application::Get();
	RenderLayer::Sptr renderLayer = app.GetLayer<RenderLayer>();

	BulletDebugMode physicsDrawMode = app.CurrentScene()->GetPhysicsDebugDrawMode();
	if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDrawMode)) {
		app.CurrentScene()->SetPhysicsDebugDrawMode(physicsDrawMode);
	}

	ImGui::Separator();

	RenderFlags flags = renderLayer->GetRenderFlags();
	bool changed = false;
	bool temp = *(flags & RenderFlags::EnableColorCorrection);
	if (ImGui::Checkbox("Enable Color Correction", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableColorCorrection) | (temp ? RenderFlags::EnableColorCorrection : RenderFlags::None);
	}

	temp = *(flags & RenderFlags::EnableWarm);
	if (ImGui::Checkbox("Enable Warm", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableWarm) | (temp ? RenderFlags::EnableWarm : RenderFlags::None);
	}

	temp = *(flags & RenderFlags::EnableCold);
	if (ImGui::Checkbox("Enable Cold", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableCold) | (temp ? RenderFlags::EnableCold : RenderFlags::None);
	}

	temp = *(flags & RenderFlags::EnableDumpster);
	if (ImGui::Checkbox("Enable Dumpster", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableDumpster) | (temp ? RenderFlags::EnableDumpster : RenderFlags::None);
	}

	if (changed) {
		renderLayer->SetRenderFlags(flags);
	}
}
