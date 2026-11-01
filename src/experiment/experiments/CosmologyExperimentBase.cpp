/**
 * @file CosmologyExperimentBase.cpp
 * @brief Shared scaffolding for the cosmology validation experiments.
 */

#include "CosmologyExperimentBase.h"

#include <cmath>
#include <limits>
#include <QVariant>

namespace quantumverse {

CosmologyCheck CosmologyExperimentBase::makeCheck(
    const QString& param, double projectVal, double referenceVal,
    const CosmologyReference& ref,
    double passRelativePct, double passSigma) const
{
    CosmologyCheck c;
    c.parameter       = param;
    c.projectValue    = projectVal;
    c.referenceValue  = referenceVal;
    c.observedValue   = ref.value;
    c.observedSigma   = ref.sigma;
    c.source          = ref.source;
    c.unit            = ref.unit;

    if (referenceVal != 0.0)
        c.relErrorPct = 100.0 * (projectVal - referenceVal) / referenceVal;
    else
        c.relErrorPct = (projectVal == 0.0) ? 0.0 : std::numeric_limits<double>::infinity();

    if (ref.sigma > 0.0 && ref.sigma >= 1e-10 * std::abs(ref.value)) {
        c.tensionSigma = (projectVal - ref.value) / ref.sigma;
    } else {
        // Analytic / exact reference (sigma ~ 0): no meaningful sigma-tension.
        c.tensionSigma = std::numeric_limits<double>::quiet_NaN();
    }

    c.pass = (ref.sigma > 0.0 && ref.sigma >= 1e-10 * std::abs(ref.value))
        ? (std::abs(c.tensionSigma) <= passSigma)
        : (std::abs(c.relErrorPct) <= passRelativePct);
    return c;
}

void CosmologyExperimentBase::recordCheck(ExperimentResult& out,
                                          const CosmologyCheck& c) const
{
    const QString base = c.parameter;
    out.scalars[base + QStringLiteral("_project")] = c.projectValue;
    out.scalars[base + QStringLiteral("_ref")]     = c.referenceValue;
    out.scalars[base + QStringLiteral("_obs")]     = c.observedValue;
    out.scalars[base + QStringLiteral("_sigma")]   = c.observedSigma;
    out.scalars[base + QStringLiteral("_source")]  = c.source;
    out.scalars[base + QStringLiteral("_unit")]    = c.unit;
    out.scalars[base + QStringLiteral("_rel_err")] = c.relErrorPct;
    out.scalars[base + QStringLiteral("_tension")] = c.tensionSigma;
    out.scalars[base + QStringLiteral("_pass")]    = c.pass;
    out.logLines << QStringLiteral("check recorded");
}

void CosmologyExperimentBase::setReferenceProvider(
    std::shared_ptr<CosmologyProvider> ref)
{
    m_reference = std::move(ref);
}

} // namespace quantumverse
