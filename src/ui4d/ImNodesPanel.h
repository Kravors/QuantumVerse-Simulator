#pragma once

#include "ui4d/ImGuiBackend.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace quantumverse {
namespace ui4d {

class ImNodesPanel {
public:
    ImNodesPanel() = default;
    ~ImNodesPanel() = default;

    void initialize();
    void shutdown();
    void render();

    void addNode(int id, const std::string& label);
    void addLink(int id, int start_attr, int end_attr);

private:
    struct Node {
        int id;
        std::string label;
        std::string type;
    };

    struct Link {
        int id;
        int start_attr;
        int end_attr;
    };

    std::vector<Node> nodes_;
    std::vector<Link> links_;
    int next_id_ = 1;
    int next_link_id_ = 1;
    bool initialized_ = false;
};

} // namespace ui4d
} // namespace quantumverse