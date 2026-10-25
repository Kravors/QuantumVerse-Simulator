#include "PaperGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QDebug>
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
    QFile file(":/paper_template.tex");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[PaperGenerator] Could not open embedded template :/paper_template.tex";
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
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
                                   const QString& llmAbstract)
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
    tex.replace("{{LLM_ABSTRACT}}", escapeLatex(llmAbstract));

    // Write .tex to output directory
    QString outDir = QDir::currentPath() + "/output";
    QDir().mkpath(outDir);

    QString texPath = outDir + "/discovery_paper.tex";
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
    process->start("pdflatex", QStringList() << "-interaction=batchmode" << "-halt-on-error" << "discovery_paper.tex");

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this, process, texPath](int exitCode, QProcess::ExitStatus) {
            if (exitCode == 0) {
                QString pdfPath = texPath;
                pdfPath.replace(".tex", ".pdf");
                qDebug() << "[PaperGenerator] PDF generated:" << pdfPath;
                emit paperGenerated(pdfPath);
            } else {
                qWarning() << "[PaperGenerator] pdflatex failed with exit code" << exitCode;
            }
            process->deleteLater();
        });
}

void PaperGenerator::generatePaper(const ScanSummary& summary,
                                   const QString& llmAbstract)
{
    generatePaper(summary.mass, summary.spin, summary.charge,
                  summary.kretschmann, summary.redshift,
                  llmAbstract);
}

} // namespace quantumverse
