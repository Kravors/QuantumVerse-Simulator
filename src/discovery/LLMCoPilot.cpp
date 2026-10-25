#include "LLMCoPilot.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QNetworkRequest>
#include <QDebug>

namespace quantumverse {

LLMCoPilot::LLMCoPilot(QObject* parent)
    : QObject(parent)
{
    m_network = new QNetworkAccessManager(this);
    connect(m_network, &QNetworkAccessManager::finished,
        this, [this](QNetworkReply* reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isNull() && doc.isObject()) {
                    QString response = doc.object()["response"].toString();
                    if (!response.isEmpty()) {
                        emit analysisReady(response);
                        reply->deleteLater();
                        return;
                    }
                }
                emit analysisReady("LLM returned an empty or unparseable response.");
            } else {
                emit analysisReady("Error connecting to LLM: " + reply->errorString());
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

    QJsonObject json;
    json["model"] = "llama3";
    json["prompt"] = prompt;
    json["stream"] = false;

    QJsonDocument doc(json);
    m_network->post(request, doc.toJson());
}

} // namespace quantumverse
