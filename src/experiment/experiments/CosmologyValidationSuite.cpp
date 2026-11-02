/**
 * @file CosmologyValidationSuite.cpp
 * @brief Meta-experiment: runs all six cosmology validation experiments.
 */

#include "CosmologyValidationSuite.h"

#include "AgeOfUniverseExperiment.h"
#include "DistanceLadderExperiment.h"
#include "CMBTemperatureExperiment.h"
#include "BAOSoundHorizonExperiment.h"
#include "HubbleEvolutionExperiment.h"
#include "GrowthFactorExperiment.h"

#include <QDateTime>

namespace quantumverse {

CosmologyValidationSuite::CosmologyValidationSuite()
    : Experiment()
{
}

QVector<ExperimentParameter> CosmologyValidationSuite::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("H0"),       QStringLiteral("H₀ (km/s/Mpc)"),
              QStringLiteral("double"), QVariant(67.66), QVariant(50.0), QVariant(90.0),
              QStringList(), QStringLiteral("km/s/Mpc"),
              QStringLiteral("Planck 67.66, SH0ES 73.04.")});
    p.append({QStringLiteral("OmegaM"),   QStringLiteral("Ωₘ (matter)"),
              QStringLiteral("double"), QVariant(0.3111), QVariant(0.1), QVariant(0.6),
              QStringList(), QStringLiteral(""), QStringLiteral("Matter density parameter.")});
    p.append({QStringLiteral("OmegaB"),   QStringLiteral("Ω_b (baryons)"),
              QStringLiteral("double"), QVariant(0.049), QVariant(0.01), QVariant(0.15),
              QStringList(), QStringLiteral(""), QStringLiteral("Baryon density parameter.")});
    p.append({QStringLiteral("OmegaR"),   QStringLiteral("Ωᵣ (radiation)"),
              QStringLiteral("double"), QVariant(9.1e-5), QVariant(0.0), QVariant(1e-3),
              QStringList(), QStringLiteral(""), QStringLiteral("Radiation density parameter.")});
    p.append({QStringLiteral("OmegaL"),   QStringLiteral("Ω_Λ (dark energy)"),
              QStringLiteral("double"), QVariant(0.6889), QVariant(0.0), QVariant(0.99),
              QStringList(), QStringLiteral(""), QStringLiteral("Dark-energy density parameter.")});
    p.append({QStringLiteral("Tcmb"),     QStringLiteral("T_CMB (K)"),
              QStringLiteral("double"), QVariant(2.7255), QVariant(2.0), QVariant(3.0),
              QStringList(), QStringLiteral("K"), QStringLiteral("CMB temperature today.")});
    p.append({QStringLiteral("passSigma"),QStringLiteral("Pass tolerance (σ)"),
              QStringLiteral("double"), QVariant(3.0), QVariant(1.0), QVariant(5.0),
              QStringList(), QStringLiteral("σ"), QStringLiteral("Tolerance for σ-referenced checks.")});
    p.append({QStringLiteral("theory_choice"),QStringLiteral("Cosmology theory to test"),
              QStringLiteral("string"), QVariant(QStringLiteral("project")),
              QVariant(), QVariant(),
              QStringList({QStringLiteral("project"), QStringLiteral("fR"),
                           QStringLiteral("wcdm"), QStringLiteral("brans_dicke")}),
              QStringLiteral(""), QStringLiteral("project = ΛCDM via FRWMetric adapter; fR = f(R) gravity; wcdm = wCDM with CPL dark energy; brans_dicke = Brans-Dicke scalar-tensor.")});
    return p;
}

namespace {
void mergeResult(ExperimentResult& out, const ExperimentResult& sub,
                 const QString& tag) {
    // Parameter names are unique across experiments, so merge scalars
    // unprefixed; only log lines carry the experiment tag.
    for (auto it = sub.scalars.cbegin(); it != sub.scalars.cend(); ++it)
        out.scalars.insert(it.key(), it.value());
    for (const auto& line : sub.logLines)
        out.logLines << QStringLiteral("[%1] %2").arg(tag, line);
}
} // namespace

ExperimentResult CosmologyValidationSuite::run(
    const QVariantMap& params,
    std::function<void(double)> progress,
    std::function<bool()> shouldAbort)
{
    ExperimentResult out;
    out.success = true;

    m_H0     = params.value(QStringLiteral("H0"),      67.66).toDouble();
    m_Om     = params.value(QStringLiteral("OmegaM"),  0.3111).toDouble();
    m_Ob     = params.value(QStringLiteral("OmegaB"),  0.049).toDouble();
    m_Or     = params.value(QStringLiteral("OmegaR"),  9.1e-5).toDouble();
    m_OL     = params.value(QStringLiteral("OmegaL"),  0.6889).toDouble();
    m_Tcmb   = params.value(QStringLiteral("Tcmb"),    2.7255).toDouble();
    m_passSigma = params.value(QStringLiteral("passSigma"), 3.0).toDouble();

    const double Ok = m_Ok();
    m_theoryChoice = params.value(QStringLiteral("theory_choice"), QStringLiteral("project")).toString();
    std::shared_ptr<CosmologyProvider> project;
    if (m_theoryChoice == QStringLiteral("fR")) {
        project = std::make_shared<FRGravityProvider>(
            m_H0, m_Om, m_Ob, m_Or, m_OL, Ok, m_Tcmb, 0.3);
    } else if (m_theoryChoice == QStringLiteral("wcdm")) {
        project = std::make_shared<WCDMProvider>(
            m_H0, m_Om, m_Ob, m_Or, m_OL, Ok, m_Tcmb, -0.9, 0.0);
    } else if (m_theoryChoice == QStringLiteral("brans_dicke")) {
        project = std::make_shared<BransDickeProvider>(
            m_H0, m_Om, m_Ob, m_Or, m_OL, Ok, m_Tcmb, 50.0);
    } else {
        project = std::make_shared<ProjectCosmologyAdapter>(
            m_H0, m_Om, m_Ob, m_Or, m_OL, Ok, m_Tcmb);
    }
    auto reference = std::make_shared<LambdaCDMReferenceProvider>(
        m_H0, m_Om, m_Ob, m_Or, m_OL, Ok, m_Tcmb);

    const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    out.scalars[QStringLiteral("suite_timestamp")] = stamp;
    out.scalars[QStringLiteral("suite_provider")]  = project->name();
    out.scalars[QStringLiteral("suite_source")]    = project->parametersSource();
    out.scalars[QStringLiteral("suite_reference")] = reference->name();
    out.scalars[QStringLiteral("suite_theory")]    = m_theoryChoice;
    if (m_theoryChoice == QStringLiteral("fR")) {
        auto fr = std::dynamic_pointer_cast<FRGravityProvider>(project);
        if (fr) out.scalars[QStringLiteral("suite_fR_b")] = fr->fR();
    }
    if (m_theoryChoice == QStringLiteral("wcdm")) {
        auto w = std::dynamic_pointer_cast<WCDMProvider>(project);
        if (w) {
            out.scalars[QStringLiteral("suite_wCDM_w0")] = w->w0();
            out.scalars[QStringLiteral("suite_wCDM_wa")] = w->wa();
        }
    }
    if (m_theoryChoice == QStringLiteral("brans_dicke")) {
        auto bd = std::dynamic_pointer_cast<BransDickeProvider>(project);
        if (bd) out.scalars[QStringLiteral("suite_BD_omega")] = bd->omega();
    }
    out.logLines << QString("Cosmology Validation Suite — %1").arg(stamp);
    out.logLines << QString("Provider: %1  (%2)   Reference: %3  (%4)")
                    .arg(project->name(), project->parametersSource(),
                         reference->name(), reference->parametersSource());

    auto runOne = [&](const QString& tag, std::unique_ptr<Experiment> exp,
                      double progressLo, double progressHi) -> bool {
        CosmologyExperimentBase* base = dynamic_cast<CosmologyExperimentBase*>(exp.get());
        if (base) base->setReferenceProvider(reference);
        double lo = progressLo, hi = progressHi;
        auto mappedProgress = [progress, lo, hi](double p) {
            if (progress) progress(lo + (hi - lo) * p);
        };
        ExperimentResult sub = exp->run(QVariantMap{}, mappedProgress, shouldAbort);
        if (shouldAbort()) { out.success = false; return false; }
        mergeResult(out, sub, tag);
        out.success = out.success && sub.success;
        return true;
    };

    const int n = 6;
    const double step = 1.0 / n;
    auto next = [&](double& cursor) { double c = cursor; cursor += step; return c; };

    double cursor = 0.0;
    runOne(QStringLiteral("age/"),        std::make_unique<AgeOfUniverseExperiment>(project),
           next(cursor), next(cursor));
    runOne(QStringLiteral("distance/"),   std::make_unique<DistanceLadderExperiment>(project),
           next(cursor), next(cursor));
    runOne(QStringLiteral("cmb/"),        std::make_unique<CMBTemperatureExperiment>(project),
           next(cursor), next(cursor));
    runOne(QStringLiteral("bao/"),        std::make_unique<BAOSoundHorizonExperiment>(project),
           next(cursor), next(cursor));
    runOne(QStringLiteral("hubble/"),     std::make_unique<HubbleEvolutionExperiment>(project),
           next(cursor), next(cursor));
    runOne(QStringLiteral("growth/"),     std::make_unique<GrowthFactorExperiment>(project),
           next(cursor), next(cursor));

    if (progress) progress(1.0);
    return out;
}

} // namespace quantumverse
