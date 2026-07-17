/**
 * @file AlertRouter.h
 * @brief Routes parsed GCN alerts to the appropriate discovery adapter
 *
 * Inspects the alert payload and forwards it to the registered
 * LIGOAdapter, IceCubeAdapter, or TESSAlertAdapter based on the
 * originating observatory.  Adapters are not owned by the router;
 * callers manage their lifetime.
 */

#ifndef QUANTUMVERSE_ALERT_ROUTER_H
#define QUANTUMVERSE_ALERT_ROUTER_H

#include <QObject>
#include <QJsonObject>

#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/GCNNoticeParser.h"
#include "data/FermiGBMAdapter.h"

namespace quantumverse {

class TESSAlertAdapter; // forward declaration
class FermiGBMAdapter;  // forward declaration

/**
 * @brief Dispatches incoming alerts to the matching multi-messenger adapter.
 */
class AlertRouter : public QObject
{
    Q_OBJECT
public:
    explicit AlertRouter(QObject* parent = nullptr);
    ~AlertRouter() override = default;

    /** @brief Register the LIGO adapter for gravitational-wave alerts. */
    void setLIGOAdapter(LIGOAdapter* adapter) { m_ligo = adapter; }

    /** @brief Register the IceCube adapter for neutrino alerts. */
    void setIceCubeAdapter(IceCubeAdapter* adapter) { m_icecube = adapter; }

    /** @brief Register the TESS adapter for exoplanet transit alerts. */
    void setTESSAdapter(TESSAlertAdapter* adapter) { m_tess = adapter; }

    /** @brief Register the Fermi GBM adapter for gamma-ray burst alerts. */
    void setFermiGBMAdapter(FermiGBMAdapter* adapter) { m_fermi_gbm = adapter; }

public slots:
    /**
     * @brief Parse and route a raw GCN Kafka JSON payload.
     * @param alert Parsed JSON object from the Kafka listener.
     */
    void routeAlert(const QJsonObject& alert);

signals:
    void alertRouted(const QString& destination);
    void routingError(const QString& reason);
    void parsedAlertReady(const QJsonObject& alert, AlertOrigin origin);

private:
    LIGOAdapter* m_ligo = nullptr;
    IceCubeAdapter* m_icecube = nullptr;
    TESSAlertAdapter* m_tess = nullptr;
    FermiGBMAdapter* m_fermi_gbm = nullptr;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ALERT_ROUTER_H
