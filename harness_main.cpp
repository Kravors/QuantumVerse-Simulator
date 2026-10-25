#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include "PaperGenerator.h"

using namespace quantumverse;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    PaperGenerator gen;

    QObject::connect(&gen, &PaperGenerator::paperGenerated,
                     [&](const QString &pdf) {
        qInfo().noquote() << "[HARNESS] paperGenerated:" << pdf;
        const QFileInfo fi(pdf);
        if (fi.exists() && fi.size() > 0) {
            qInfo().noquote() << "[HARNESS] SUCCESS size=" << fi.size();
            app.exit(0);
        } else {
            qWarning().noquote() << "[HARNESS] FAILED: pdf missing or empty";
            app.exit(3);
        }
    });

    QTimer::singleShot(120000, [&]{
        qCritical() << "[HARNESS] TIMEOUT after 120s";
        app.exit(2);
    });

    const double mass     = 1.0;
    const double spin     = 0.999;
    const double charge   = 0.0;
    const double kretch   = 1e-6;
    const double redshift = 0.1;
    const QString llmAbstract =
        QStringLiteral("We report a near-extremal Kerr black hole candidate "
                       "identified by QuantumVerse's discovery scan. "
                       "The spin_parameter is 0.999 and the confidence is 50%. "
                       "Harness test abstract with LaTeX specials: _ % & # $ { } ~ ^ \\.");

    QTimer::singleShot(0, [&]{
        qInfo() << "[HARNESS] calling generatePaper(...)";
        gen.generatePaper(mass, spin, charge, kretch, redshift, llmAbstract);
    });

    return app.exec();
}
