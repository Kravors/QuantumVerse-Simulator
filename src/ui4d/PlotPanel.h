#pragma once

#include "glad.h"
#include "imgui.h"
#include "implot.h"
#include <vector>
#include <string>

namespace ui4d {

class PlotPanel {
public:
    PlotPanel(const std::string& title);
    ~PlotPanel() = default;

    void setTitle(const std::string& title);
    void render();

    void addDataPoint(double x, double y);
    void clear();

    void setXLabel(const std::string& label);
    void setYLabel(const std::string& label);

    static void showCurvaturePlot(const std::string& title, const std::vector<double>& times, const std::vector<double>& kretschmann);
    static void showAnomalyPlot(const std::string& title, const std::vector<double>& steps, const std::vector<double>& confidence);

private:
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    std::vector<double> m_xData;
    std::vector<double> m_yData;
};

}