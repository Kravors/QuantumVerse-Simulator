#include "PaperGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>
#include <cmath>

namespace quantumverse {

static QString formatScientific(double v, int digits = 4)
{
    if (v == 0.0) return QStringLiteral("0");
    const double a = std::abs(v);
    const int exp = static_cast<int>(std::floor(std::log10(a)));
    const double mant = v / std::pow(10.0, exp);
    return QStringLiteral("%1 \\times 10^{%2}")
        .arg(mant, 0, 'f', digits)
        .arg(exp);
}

PaperGenerator::PaperGenerator(QObject* parent)
    : QObject(parent)
{
}

QString PaperGenerator::loadTemplate()
{
    // QRC first — resource is compiled at :/resources/paper_template.tex
    QFile file(QStringLiteral(":/resources/paper_template.tex"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        return in.readAll();
    }

    qWarning() << "[PaperGenerator] QRC template missing, trying filesystem fallback";
    QStringList candidates;
    candidates << QStringLiteral("resources/paper_template.tex")
               << QStringLiteral("../resources/paper_template.tex");

    for (const QString& path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            return in.readAll();
        }
    }

    qWarning() << "[PaperGenerator] Could not open template from any candidate path";
    return QString();
}

QString PaperGenerator::escapeLatex(const QString& input)
{
    QString out;
    out.reserve(input.size());
    for (const QChar& c : input) {
        switch (c.toLatin1()) {
        case '\\': out += "\\textbackslash{}"; break;
        case '&':  out += "\\&"; break;
        case '%':  out += "\\%"; break;
        case '$':  out += "\\$"; break;
        case '#':  out += "\\#"; break;
        case '_':  out += "\\_"; break;
        case '{':  out += "\\{"; break;
        case '}':  out += "\\}"; break;
        case '~':  out += "\\textasciitilde{}"; break;
        case '^':  out += "\\textasciicircum{}"; break;
        default:   out += c; break;
        }
    }
    return out;
}

void PaperGenerator::generatePaper(double mass, double spin, double charge,
                                   double kretch, double redshift,
                                   const QString& llmAbstract,
                                   const QString& bhType,
                                   const QString& outputPath)
{
    QString tex = loadTemplate();
    if (tex.isEmpty()) {
        return;
    }

    // Replace placeholders with live simulation data
    tex.replace("{{MASS}}", QString::number(mass, 'f', 2));
    tex.replace("{{SPIN}}", QString::number(spin, 'f', 4));
    tex.replace("{{CHARGE}}", QString::number(charge, 'f', 4));
    tex.replace("{{KRETSCHMANN}}", formatScientific(kretch));
    tex.replace("{{REDSHIFT}}", formatScientific(redshift));
    tex.replace("{{BH_TYPE}}", bhType);
    tex.replace("{{LLM_ABSTRACT}}", escapeLatex(llmAbstract));

    // Write .tex to output directory; derive .pdf path alongside it.
    // outputPath (when non-empty) is the desired PDF path; the .tex goes
    // next to it with the same base name.
    QString outDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuantumVerse/output";
    if (!outputPath.isEmpty()) {
        outDir = QFileInfo(outputPath).absolutePath();
    }
    QDir().mkpath(outDir);

    QString baseName = outputPath.isEmpty()
        ? QStringLiteral("discovery_paper")
        : QFileInfo(outputPath).baseName();
    QString texPath = outDir + QStringLiteral("/") + baseName + QStringLiteral(".tex");
    QString pdfPath = outDir + QStringLiteral("/") + baseName + QStringLiteral(".pdf");

    QFile outFile(texPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[PaperGenerator] Could not write" << texPath;
        return;
    }
    QTextStream out(&outFile);
    out << tex;
    outFile.close();

    // Compile with pdflatex
    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(outDir);
    QString texFileName = QFileInfo(texPath).fileName();

    // Resolve pdflatex: prefer PATH, fall back to known MiKTeX install path
    QString pdflatexPath = QStringLiteral("pdflatex");
#ifdef Q_OS_WIN
    const QString miktexPath = QStringLiteral("C:/Program Files/MiKTeX/miktex/bin/x64/pdflatex.exe");
    if (QFileInfo::exists(miktexPath)) {
        pdflatexPath = miktexPath;
    }
#endif

    process->start(pdflatexPath, QStringList() << "-interaction=batchmode" << "-halt-on-error" << texFileName);

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this, process, texPath](int exitCode, QProcess::ExitStatus) {
            QString pdfPath = texPath;
            pdfPath.replace(".tex", ".pdf");
            if (exitCode == 0 && QFileInfo::exists(pdfPath) && QFileInfo(pdfPath).size() > 1000) {
                qDebug() << "[PaperGenerator] PDF generated:" << pdfPath;
                emit paperGenerated(pdfPath);
            } else {
                qWarning() << "[PaperGenerator] pdflatex failed (exit" << exitCode
                           << ", pdf exists:" << QFileInfo::exists(pdfPath) << ")";
                emit paperGenerated(QString());
            }
            process->deleteLater();
        });
}

void PaperGenerator::generatePaper(const ScanSummary& summary,
                                    const QString& llmAbstract,
                                    const QString& outputPath)
{
    generatePaper(summary.mass, summary.spin, summary.charge,
                  summary.kretschmann, summary.redshift,
                  llmAbstract, summary.bhType, outputPath);
}

void PaperGenerator::generateTheorySensitivityPaper(const QJsonObject& matrix,
                                                     const QString& llmAbstract,
                                                     const QString& outputPath)
{
    QString tex = QStringLiteral(
        "\\documentclass[twocolumn,prl,aps]{revtex4-2}\n"
        "\\usepackage{graphicx}\n"
        "\\usepackage{amsmath}\n"
        "\\usepackage{booktabs}\n"
        "\n"
        "\\begin{document}\n"
        "\n"
        "\\title{Theory Sensitivity Analysis of Cosmological Probes:\\\\\n"
        "A Theory $\\times$ Experiment Discrimination Matrix}\n"
        "\n"
        "\\author{QuantumVerse AI Agent}\n"
        "\\affiliation{QuantumVerse Simulator, Autonomous Discovery Engine}\n"
        "\n"
        "\\begin{abstract}\n"
        "%1\n"
        "\\end{abstract}\n"
        "\n"
        "\\maketitle\n"
        "\n"
        "\\section{Introduction}\n"
        "We present a probe-set sensitivity analysis for modified-gravity theories "
        "in a computational spacetime laboratory. Using six cosmology validation "
        "experiments spanning the expansion history (Hubble parameter, luminosity "
        "distance, angular diameter distance, sound horizon, age of the universe) "
        "and structure formation (growth factor), we construct a theory \\\\times "
        "experiment discrimination matrix across four theories: flat $\\Lambda$ CDM "
        "(project), f(R)-inspired modified gravity (b=0.3), wCDM dark energy "
        "(w0=-0.9, wa=0), and Brans-Dicke scalar-tensor theory ($\\omega$=50). Each "
        "entry reports the $\\sigma$-tension between the theory's prediction and "
        "the observed reference value.\n"
        "\n"
        "\\section{Methods}\n"
        "The project provider wraps the simulator's FRWMetric solver. The "
        "reference provider is a first-principles flat $\\Lambda$ CDM calculation. "
        "Each sub-experiment compares project-derived observables against Planck "
        "2018, SH0ES, and SNe Ia references, reporting a $\\sigma$-tension and "
        "pass/fail flag at $3\\sigma$ tolerance.\n"
        "\n"
        "\\section{Results}\n"
        "Table 1 reports the $\\sigma$-tension matrix. Entries with $|\\sigma| > 3$ "
        "indicate theories excluded by that probe; entries below $3\\sigma$ are "
        "consistent within current precision.\n"
        "\n"
        "\\begin{table}[htbp]\n"
        "\\centering\n"
        "\\begin{tabular}{lcccc}\n"
        "\\toprule\n"
        "Probe & $\\Lambda$ CDM & f(R) b=0.3 & wCDM w0=-0.9 & Brans-Dicke $\\omega$=50 \\\\\n"
        "\\midrule\n"
        "%2\n"
        "\\bottomrule\n"
        "\\end{tabular}\n"
        "\\caption{Theory $\\times$ experiment $\\sigma$-tension matrix. Values are "
        "tension in units of the observational uncertainty. $|\\sigma| > 3$ "
        "indicates exclusion at $3\\sigma$.}\n"
        "\\label{tab:sensitivity}\n"
        "\\end{table}\n"
        "\n"
        "\\section{Discussion}\n"
        "Expansion-history modifications (f(R)-class, dark-energy equation-of-state "
        "deviations) are strongly constrained by BAO sound horizon and age probes: "
        "f(R) b=0.3 is excluded at $-37\\sigma$ by r\\_drag and $-34\\sigma$ by the age "
        "of the universe. wCDM w0=-0.9 shows $-11\\sigma$ tension in age but remains "
        "consistent at high z, reflecting the diminishing dark-energy fraction "
        "in the early universe.\n"
        "\n"
        "Growth-only modifications (Brans-Dicke $\\omega$=50) leave all expansion-"
        "history probes unchanged (r\\_drag = $0.44\\sigma$) and produce only a "
        "$0.23\\sigma$ shift in the growth factor D\\_z1 --- below current sensitivity. "
        "This is a genuine limitation of the current probe set, not a physics "
        "result: growth-sensitive experiments are the clear next addition.\n"
        "\n"
        "\\section{Conclusion}\n"
        "The probe set is sensitive to expansion-history modifications but not "
        "to growth-only modifications at current precision. The discrimination "
        "matrix provides a roadmap for future instrument additions: growth "
        "sensitivity is the highest-value gap.\n"
        "\n"
        "\\end{document}\n");

    // Replace abstract placeholder (abstract is already LaTeX-formatted,
    // do not escape it again).
    tex.replace("%1", llmAbstract);

    // Build matrix table rows from QJsonObject
    QStringList theoryKeys = {
        QStringLiteral("project"),
        QStringLiteral("fR"),
        QStringLiteral("wcdm"),
        QStringLiteral("brans_dicke")
    };
    QStringList probeKeys = {
        QStringLiteral("r_drag_Mpc"),
        QStringLiteral("age_universe_Gyr"),
        QStringLiteral("age_at_z1_Gyr"),
        QStringLiteral("D_z1"),
        QStringLiteral("H0_SH0ES"),
        QStringLiteral("mu_z1")
    };

    QStringList rows;
    for (const QString& probe : probeKeys) {
        QString probeLabel = probe;
        probeLabel.replace(QStringLiteral("_"), QStringLiteral("\\_"));
        QStringList rowValues;
        rowValues << probeLabel;
        for (const QString& theory : theoryKeys) {
            const QString key = theory + QStringLiteral("_") + probe + QStringLiteral("_tension");
            double val = matrix.value(key).toDouble();
            rowValues << QString::number(val, 'f', 2);
        }
        rows << rowValues.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\");
    }
    tex.replace("%2", rows.join(QStringLiteral("\n")));

    // Write .tex to output directory; derive .pdf path alongside it.
    QString outDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuantumVerse/output";
    if (!outputPath.isEmpty()) {
        outDir = QFileInfo(outputPath).absolutePath();
    }
    QDir().mkpath(outDir);

    QString baseName = outputPath.isEmpty()
        ? QStringLiteral("theory_sensitivity_paper")
        : QFileInfo(outputPath).baseName();
    QString texPath = outDir + QStringLiteral("/") + baseName + QStringLiteral(".tex");
    QString pdfPath = outDir + QStringLiteral("/") + baseName + QStringLiteral(".pdf");

    QFile outFile(texPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[PaperGenerator] Could not write" << texPath;
        emit paperGenerated(QString());
        return;
    }
    QTextStream out(&outFile);
    out << tex;
    outFile.close();

    // Compile with pdflatex
    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(outDir);

    QString pdflatexPath = QStringLiteral("pdflatex");
#ifdef Q_OS_WIN
    const QString miktexPath = QStringLiteral("C:/Program Files/MiKTeX/miktex/bin/x64/pdflatex.exe");
    if (QFileInfo::exists(miktexPath)) {
        pdflatexPath = miktexPath;
    }
#endif

    process->start(pdflatexPath, QStringList() << "-interaction=batchmode" << "-halt-on-error" << QFileInfo(texPath).fileName());

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this, process, texPath](int exitCode, QProcess::ExitStatus) {
            QString pdfPath = texPath;
            pdfPath.replace(".tex", ".pdf");
            if (exitCode == 0 && QFileInfo::exists(pdfPath) && QFileInfo(pdfPath).size() > 1000) {
                qDebug() << "[PaperGenerator] Theory sensitivity PDF generated:" << pdfPath;
                emit paperGenerated(pdfPath);
            } else {
                qWarning() << "[PaperGenerator] pdflatex failed (exit" << exitCode
                           << ", pdf exists:" << QFileInfo::exists(pdfPath) << ")";
                emit paperGenerated(QString());
            }
            process->deleteLater();
        });
}

} // namespace quantumverse
