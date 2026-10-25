#include "LLMCoPilot.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QNetworkRequest>
#include <QDebug>

namespace quantumverse {

LLMCoPilot::LLMCoPilot(QObject* parent)
    : QObject(parent)
    , m_pendingSuccess(nullptr)
    , m_pendingFailure(nullptr)
{
    m_network = new QNetworkAccessManager(this);
    connect(m_network, &QNetworkAccessManager::finished,
        this, [this](QNetworkReply* reply) {
            auto success = m_pendingSuccess;
            auto failure = m_pendingFailure;
            m_pendingSuccess = nullptr;
            m_pendingFailure = nullptr;

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isNull() && doc.isObject()) {
                    QString response = doc.object()["response"].toString();
                    if (!response.isEmpty() && success) {
                        (this->*success)(response);
                        reply->deleteLater();
                        return;
                    }
                }
                if (failure) {
                    (this->*failure)("LLM returned an empty or unparseable response.");
                }
            } else {
                if (failure) {
                    (this->*failure)("Error connecting to LLM: " + reply->errorString());
                }
            }
            reply->deleteLater();
        });
}

void LLMCoPilot::analyzeAnomaly(const QString& anomalyData)
{
    QUrl url("http://localhost:11434/api/generate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString prompt = "You are a brilliant astrophysicist analyzing a 4D spacetime simulation. "
                     "The following data was logged by the QuantumVerse Discovery Engine: "
                     + anomalyData + ". "
                     "Explain what this means for General Relativity in 2-3 sentences. "
                     "Be precise, use scientific terms (metric signature, Kretschmann scalar, "
                     "cosmic censorship, event horizon, naked singularity), and focus on "
                     "spacetime curvature and physical validity.";

    m_pendingSuccess = &LLMCoPilot::analysisReady;
    m_pendingFailure = &LLMCoPilot::analysisReady;

    QJsonObject json;
    json["model"] = "llama3";
    json["prompt"] = prompt;
    json["stream"] = false;

    QJsonDocument doc(json);
    m_network->post(request, doc.toJson());
}

void LLMCoPilot::generatePaperAbstract(const QJsonObject& scanData)
{
    QUrl url("http://localhost:11434/api/generate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString prompt = "You are writing the abstract of a physics discovery paper. "
                     "Given the following scan results as JSON, write a 120-180 word abstract "
                     "in the style of Physical Review Letters. Do not invent numbers; use only "
                     "the values in the JSON. Do not include equations. End with one sentence "
                     "stating the discovery's significance.\n\n"
                     "Scan data: " + QString(QJsonDocument(scanData).toJson(QJsonDocument::Compact)) + "\n\n"
                     "Respond with only the abstract text, no preamble.";

    m_pendingSuccess = &LLMCoPilot::abstractReady;
    m_pendingFailure = &LLMCoPilot::abstractFailed;

    QJsonObject json;
    json["model"] = "llama3";
    json["prompt"] = prompt;
    json["stream"] = false;

    QJsonDocument doc(json);
    m_network->post(request, doc.toJson());
}

} // namespace quantumverse
