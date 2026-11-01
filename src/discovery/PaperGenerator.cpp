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

    // Write .tex to output directory
    QString outDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuantumVerse/output";
    if (!outputPath.isEmpty()) {
        outDir = QFileInfo(outputPath).absolutePath();
    }
    QDir().mkpath(outDir);

    QString texPath = outputPath.isEmpty()
        ? outDir + "/discovery_paper.tex"
        : outputPath;
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

} // namespace quantumverse
