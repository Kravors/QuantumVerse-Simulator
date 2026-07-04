#include "ui4d/ImNodesPanel.h"
#include "third_party/imnodes.h"
#include <imgui.h>

namespace quantumverse {
namespace ui4d {

void ImNodesPanel::initialize() {
    if (!initialized_) {
        imnodes::Initialize();
        initialized_ = true;
    }
}

void ImNodesPanel::shutdown() {
    if (initialized_) {
        imnodes::Shutdown();
        initialized_ = false;
    }
}

void ImNodesPanel::render() {
    if (!initialized_) return;

    ImGui::Begin("Causal Graph");
    
    imnodes::BeginNodeEditor();

    for (const auto& node : nodes_) {
        imnodes::BeginNode(node.id);
        ImGui::Text("%s", node.label.c_str());
        imnodes::BeginOutputAttribute(node.id * 10);
        ImGui::Text("->");
        imnodes::EndAttribute();
        imnodes::EndNode();
    }

    for (const auto& link : links_) {
        imnodes::Link(link.id, link.start_attr, link.end_attr);
    }

    imnodes::EndNodeEditor();

    ImGui::End();
}

void ImNodesPanel::addNode(int id, const std::string& label) {
    nodes_.push_back({id, label, "event"});
}

void ImNodesPanel::addLink(int id, int start_attr, int end_attr) {
    links_.push_back({id, start_attr, end_attr});
}

} // namespace ui4d
} // namespace quantumverse