/**
 * @file PaperGenerator.h
 * @brief Generates publication-ready LaTeX papers from validated AI discoveries.
 *
 * PaperGenerator reads a LaTeX template, substitutes live simulation data
 * (mass, spin, charge, Kretschmann scalar, redshift, LLM abstract), writes
 * the .tex file, and compiles it to PDF via pdflatex.
 */
#pragma once

#include <QObject>
#include <QString>

#include "discovery/ScanSummary.h"

namespace quantumverse {

/**
 * @brief Automated scientific paper generation from validated discoveries.
 *
 * When the RL agent finds a stable near-extremal black hole and the LLM
 * Co-Pilot writes an explanation, PaperGenerator produces a Physical Review
 * Letters-style PDF containing the metric parameters, curvature invariants,
 * and the AI-generated abstract.
 */
class PaperGenerator : public QObject {
    Q_OBJECT

public:
    explicit PaperGenerator(QObject* parent = nullptr);

    /// Generate a LaTeX paper and compile to PDF from validated discovery data.
    Q_INVOKABLE void generatePaper(double mass, double spin, double charge,
                                   double kretch, double redshift,
                                   const QString& llmAbstract);

    /// Generate a LaTeX paper and compile to PDF from a ScanSummary + LLM abstract.
    Q_INVOKABLE void generatePaper(const ScanSummary& summary,
                                   const QString& llmAbstract);

signals:
    /// Emitted with the path to the generated PDF on success.
    void paperGenerated(const QString& pdfPath);

public:
    /// @brief Escape LaTeX special characters in a string.
    /// Pure string manipulation — no QProcess, no pdflatex. Safe to unit test.
    static QString escapeLatex(const QString& input);

private:
    QString loadTemplate();
};

} // namespace quantumverse
