#include "PlotPanel.h"

namespace ui4d {

PlotPanel::PlotPanel(const std::string& title)
    : m_title(title)
    , m_xLabel("X")
    , m_yLabel("Y")
{
}

void PlotPanel::setTitle(const std::string& title) {
    m_title = title;
}

void PlotPanel::render() {
    if (ImGui::Begin(m_title.c_str())) {
        if (ImPlot::BeginPlot(m_title.c_str())) {
            ImPlot::SetupAxes(m_xLabel.c_str(), m_yLabel.c_str());
            if (!m_xData.empty() && !m_yData.empty()) {
                ImPlot::PlotLine("Data", m_xData.data(), m_yData.data(), m_xData.size());
            }
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

void PlotPanel::addDataPoint(double x, double y) {
    m_xData.push_back(x);
    m_yData.push_back(y);
}

void PlotPanel::clear() {
    m_xData.clear();
    m_yData.clear();
}

void PlotPanel::setXLabel(const std::string& label) {
    m_xLabel = label;
}

void PlotPanel::setYLabel(const std::string& label) {
    m_yLabel = label;
}

void PlotPanel::showCurvaturePlot(const std::string& title, const std::vector<double>& times, const std::vector<double>& kretschmann) {
    if (ImGui::Begin(title.c_str())) {
        if (ImPlot::BeginPlot("Kretschmann Scalar")) {
            ImPlot::SetupAxes("Time", "K");
            ImPlot::PlotLine("K", times.data(), kretschmann.data(), times.size());
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

void PlotPanel::showAnomalyPlot(const std::string& title, const std::vector<double>& steps, const std::vector<double>& confidence) {
    if (ImGui::Begin(title.c_str())) {
        if (ImPlot::BeginPlot("Anomaly Confidence")) {
            ImPlot::SetupAxes("Step", "Confidence");
            ImPlot::PlotLine("Confidence", steps.data(), confidence.data(), steps.size());
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

}