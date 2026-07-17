/**
 * @file main_qml.cpp
 * @brief Entry point for QuantumVerse Simulator with QML/Qt Quick GUI
 *
 * Initializes Qt Quick with an OpenGL viewport for 4D spacetime visualization.
 * Uses QQuickFramebufferObject for hardware-accelerated rendering.
 */

// glad.h MUST be included before any Qt OpenGL headers to prevent
// system <GL/gl.h> from conflicting with glad's OpenGL loader.
#include "../third_party/glad/glad.h"

// Define NOMINMAX before including windows.h to prevent min/max macro conflicts
#define NOMINMAX
#include <windows.h>  // for MessageBoxA
#undef connect

// Qt headers MUST come before project headers to avoid namespace pollution.
// Qt uses QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros internally.
#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QResource>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QWidget>
#include <QWindow>
#include <vector>
#include <iostream>

#include "qmlglviewport.h"
#include "qmlcamcontroller.h"
#include "scenegraphmodel.h"
#include "ui4d/SceneGraphManager.h"
#include "ui4d/UI4D.h"
#include "ui4d/PlanckMicroscope.h"
#include "ui4d/Camera4DAdapter.h"
#include "spacetime/MetricTensor.h"
#include "rendering/CurvatureRenderer.h"
#include "rendering/QuantumGeometryRenderer.h"
#include "rendering/CelestialBodyRenderer.h"
#include "rendering/GLDebug.h"
#include "physics/SingularityHandler.h"
#include "quantumgravity/CDTEngine.h"
#include "discovery/DiscoveryPanelManager.h"
#include "discovery/FindingsModel.h"
#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/TESSAlertAdapter.h"
#include "data/FermiGBMAdapter.h"
#include "data/AlertToFinding.h"
#include "data/AlertRouter.h"
#include "data/KafkaAlertListener.h"
#include "data/GCNNoticeParser.h"
#include "data/GCNReplayStream.h"
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "discovery/GalacticRotationCurveScanner.h"
#include "discovery/FineStructureConstantDriftObservatory.h"
#include "discovery/BosonStarCollisionPredictor.h"
#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "discovery/UltralightDMWaveInterferometer.h"
#include "discovery/BlackHoleJetAnomalyRecogniser.h"
#include "discovery/PrimordialLithiumCrisisSolver.h"
#include "discovery/GalacticTidalStreamCartographer.h"
#include "discovery/RecombinationConstantVariationImager.h"
#include "discovery/CMBLensingScanner.h"
#include "discovery/PTAScanner.h"
#include "discovery/FRBDispersionScanner.h"
#include "discovery/CosmicShearScanner.h"
#include "discovery/GWMemoryDetector.h"
#include "discovery/GWRingdownScanner.h"
#include "utils/TraceLogger.h"
#include "utils/CrashHandler.h"
#include "utils/FrameDiagnostics.h"

/**
 * @brief Register QML types for the QuantumVerse module
 */
void registerQmlTypes()
{
    qmlRegisterType<quantumverse::QmlGlViewport>(
        "QuantumVerse", 1, 0, "QmlGlViewport");
    qmlRegisterType<quantumverse::QmlCamController>(
        "QuantumVerse", 1, 0, "QmlCamController");
    qmlRegisterType<quantumverse::SceneGraphModel>(
        "QuantumVerse", 1, 0, "SceneGraphModel");

    // Register uncreatable types (singletons, etc.)
    qmlRegisterUncreatableType<quantumverse::SceneGraphManager>(
        "QuantumVerse", 1, 0, "SceneGraphManager",
        "SceneGraphManager is created in C++");
}

/**
 * @brief Set up OpenGL format with required settings
 */
QSurfaceFormat createSurfaceFormat()
{
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // MSAA
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    // DISABLED: Debug context causes GPU driver breakpoints after ~25 minutes
    // format.setOption(QSurfaceFormat::DebugContext, true);  // Enable debug context for GL callback
    return format;
}

int main(int argc, char* argv[])
{
    std::ofstream startupLog("startup.log");
    startupLog << "main() started at " << std::time(nullptr) << std::endl;
    startupLog.close();

    // Log everything immediately to stderr so it appears even if Qt is broken
    std::cerr << "QML main started" << std::endl;
    std::cerr.flush();

    // Early threading test - detect if C++ runtime threading support is broken
    try {
        std::thread([]() {}).join();
    } catch (const std::system_error& e) {
        std::cerr << "FATAL: Threading support not available: " << e.what() << std::endl;
        std::cerr.flush();
        MessageBoxA(nullptr, e.what(), "QuantumVerse Fatal Error: Threading Support Missing", MB_ICONERROR);
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "FATAL: Early initialization error: " << e.what() << std::endl;
        std::cerr.flush();
        MessageBoxA(nullptr, e.what(), "QuantumVerse Fatal Error", MB_ICONERROR);
        return -1;
    }

    // Headless detection - exit gracefully if no display (for CI environments)
    // But if --frames is specified, attempt headless benchmark rendering below.
    int headlessFrames = 0;
    bool autoScan = false;
    bool enableGeodesics = false;
    bool glStrict = false;
    QString screenshotPath;
    QString frameTimesPath;
    QString diagnosticsPath;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            headlessFrames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshotPath = argv[++i];
        } else if (strcmp(argv[i], "--dump-frame-times") == 0 && i + 1 < argc) {
            frameTimesPath = argv[++i];
        } else if (strcmp(argv[i], "--diagnostics") == 0 && i + 1 < argc) {
            diagnosticsPath = argv[++i];
        } else if (strcmp(argv[i], "--gl-strict") == 0) {
            glStrict = true;
        } else if (strcmp(argv[i], "--autoscan") == 0) {
            autoScan = true;
        } else if (strcmp(argv[i], "--enable-geodesics") == 0) {
            enableGeodesics = true;
        }
    }
    if (!screenshotPath.isEmpty() && headlessFrames <= 0) {
        headlessFrames = 1;
    }
    if (GetSystemMetrics(SM_CMONITORS) == 0 && headlessFrames <= 0) {
        std::cerr << "No display detected. Exiting gracefully (headless environment)." << std::endl;
        std::cerr.flush();
        return 0;
    }

    quantumverse::utils::FrameDiagnostics frameDiagnostics;
    if (!diagnosticsPath.isEmpty()) {
        frameDiagnostics.setEnabled(true);
        std::cerr << "Frame diagnostics enabled, output: " << diagnosticsPath.toStdString() << std::endl;
        std::cerr.flush();
    }

    try {
        // Initialize crash handler and trace logger first (before any other initialization)
        // NOTE: TraceLogger disabled for testing - it was causing crashes in headless environment
        // quantumverse::utils::TraceLogger::instance().initialize("quantumverse_qml.log");
        // TRACE_INIT("Starting QuantumVerse QML application");
        std::cerr << "Starting QuantumVerse QML application" << std::endl;
        std::cerr.flush();

    // Force desktop OpenGL backend before any Qt window is created.
    // On Windows, Qt 6 defaults to ANGLE (OpenGL ES -> DirectX translation),
    // which causes GL_INVALID_OPERATION on raw glClear/glDrawElements calls.
    // Environment variables must be set first, followed by the application attributes,
    // then the explicit graphics API override. QSG_OPENGL_LEGACY tells the RHI
    // backend to use classic OpenGL calls instead of the modern RHI pipeline.
    qputenv("QSG_RHI_BACKEND", QByteArray("opengl"));
    qputenv("QT_OPENGL", QByteArray("desktop"));
    qputenv("QSG_OPENGL_LEGACY", QByteArray("1"));
    qputenv("QT_ANGLE_PLATFORM", QByteArray("none"));

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    qDebug() << "QSG_RHI_BACKEND =" << qgetenv("QSG_RHI_BACKEND");
    qDebug() << "QSG_OPENGL_LEGACY =" << qgetenv("QSG_OPENGL_LEGACY");
    qDebug() << "QT_OPENGL =" << qgetenv("QT_OPENGL");
    qDebug() << "QT_ANGLE_PLATFORM =" << qgetenv("QT_ANGLE_PLATFORM");

    // Force desktop OpenGL at the application level as well.
    // This influences RHI backend selection even when setGraphicsApi() alone is insufficient.
    QGuiApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // Ensure Qt can find its plugins and QML modules when running from a
    // deployment directory that does not preserve the original Qt layout.
    qputenv("QT_PLUGIN_PATH", QByteArray("F:/qt/6.11.1/msvc2022_64/plugins"));
    qputenv("QML2_IMPORT_PATH", QByteArray("F:/qt/6.11.1/msvc2022_64/qml"));
    qDebug() << "QT_PLUGIN_PATH set to:" << qgetenv("QT_PLUGIN_PATH").constData();
    qDebug() << "QML2_IMPORT_PATH set to:" << qgetenv("QML2_IMPORT_PATH").constData();


    QSurfaceFormat::setDefaultFormat(createSurfaceFormat());

    QApplication app(argc, argv);
    app.setApplicationName("QuantumVerse Simulator");
    app.setOrganizationName("QuantumVerse");

    std::ofstream("graphics_api.txt") << "Graphics API after QApplication/QQuickWindow setup: " << QQuickWindow::graphicsApi() << std::endl;
    qDebug() << "Graphics API after QApplication/QQuickWindow setup:" << QQuickWindow::graphicsApi();

        // 1. Ensure Qt can find its plugins
        std::cerr << "QT_PLUGIN_PATH = " << qgetenv("QT_PLUGIN_PATH").constData() << std::endl;
        std::cerr << "QML2_IMPORT_PATH = " << qgetenv("QML2_IMPORT_PATH").constData() << std::endl;
        std::cerr.flush();

        // 2. Check that main.qml exists in resources
        // Note: main.qml is embedded in resources.qrc, so we check the resource system
        std::cerr << "Checking QML resources..." << std::endl;
        std::cerr.flush();

        // Set OpenGL format before creating any windows
        QSurfaceFormat::setDefaultFormat(createSurfaceFormat());

        // Register QML types
        registerQmlTypes();

        // Log startup progress
        qDebug() << "QuantumVerse: Starting QML application...";
        qDebug() << "QuantumVerse: OpenGL format set to 4.5 Core Profile";
        // TRACE_INIT("QML types registered, creating physics components");
        std::cerr << "QML types registered, creating physics components" << std::endl;
        std::cerr.flush();

        // Create the physics engine components
        double blackHoleMass = 10.0 * 1.989e30;  // 10 solar masses in kg
        auto metric = std::make_shared<quantumverse::SchwarzschildMetric>(blackHoleMass);
        auto curvatureRenderer = std::make_shared<quantumverse::CurvatureRenderer>(
            30, 100.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        curvatureRenderer->setMetric(metric);

        // Create a Schwarzschild black hole singularity (M = 10 solar masses)
        // Position at origin for the central viewport focus
        // (blackHoleMass is defined above, where the metric is created)
        std::array<double, 3> bhPos = {0.0, 0.0, 0.0};
        auto singularity = std::make_shared<quantumverse::SingularityHandler>(
            quantumverse::SingularityType::SCHWARZSCHILD,
            blackHoleMass,
            0.0,   // angular momentum
            0.0,   // charge
            bhPos
        );
        qDebug() << "QuantumVerse: Created Schwarzschild BH, r_s ="
                 << singularity->getProperties().schwarzschild_radius << "m";

        // Add singularity to curvature renderer for grid deformation
        curvatureRenderer->addSingularity(singularity);

        // Create quantum geometry renderer with CDT engine for Phase 0
        auto quantumRenderer = std::make_shared<quantumverse::QuantumGeometryRenderer>();
        auto cdtEngine = std::make_shared<quantumverse::CDTEngine>();
        quantumRenderer->setCDTEngine(cdtEngine);
        quantumRenderer->setCurrentType(quantumverse::QuantumGeometryType::CDT);
        quantumRenderer->setViewScale(1.0);

        // Create the scene graph manager
        auto sceneGraphManager = std::make_shared<quantumverse::SceneGraphManager>(metric);
        sceneGraphManager->createSolarSystem();

        // Create the Discovery Panel Manager and register all instruments
        auto discoveryPanelManager = std::make_shared<quantumverse::DiscoveryPanelManager>();
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::ExoplanetaryTTVFifthForceHunter>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::GalacticRotationCurveScanner>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::FineStructureConstantDriftObservatory>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::BosonStarCollisionPredictor>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::NeutronStarGlitchPhaseDetector>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::UltralightDMWaveInterferometer>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::BlackHoleJetAnomalyRecogniser>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::PrimordialLithiumCrisisSolver>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::GalacticTidalStreamCartographer>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::RecombinationConstantVariationImager>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::CMBLensingScanner>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::PTAScanner>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::FRBDispersionScanner>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::CosmicShearScanner>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::GWMemoryDetector>());
        discoveryPanelManager->registerInstrument(std::make_unique<quantumverse::GWRingdownScanner>());
        qDebug() << "QuantumVerse: Registered" << discoveryPanelManager->instrumentCount() << "discovery instruments";

        // Provide metric and location to the discovery panel manager
        discoveryPanelManager->setMetric(metric);
        quantumverse::Event4D origin;
        origin.t = 0.0;
        origin.x = 0.0;
        origin.y = 0.0;
        origin.z = 0.0;
        discoveryPanelManager->setLocation(origin);

        // Create the UI4D instance
        auto ui4d = std::make_shared<quantumverse::UI4D>();
        ui4d->setMetric(metric);
        ui4d->setCurvatureRenderer(curvatureRenderer);
        ui4d->setQuantumRenderer(quantumRenderer);

        // Visualization scale: object positions are stored in raw meters
        // (Sun at origin, Neptune at ~30 AU). The viewport grid spans only
        // +/-50 units, so meter-scale positions render off-grid. Map the
        // solar system into the grid (1 AU -> kViewportUnitsPerAU units) and
        // reuse the exact same factor for camera focus (exposed to QML below)
        // so the camera target and the rendered bodies always agree.
        constexpr double kAstronomicalUnit = 1.495978707e11; // meters
        constexpr double kViewportUnitsPerAU = 1.3;
        const double kViewportScale = kViewportUnitsPerAU / kAstronomicalUnit;
        ui4d->setSolarSystemScale(kViewportScale);
        std::cerr << "QuantumVerse: Setting active quantum theory to CDT" << std::endl;
        std::cerr.flush();
        ui4d->setActiveQuantumTheory("CDT");
        std::cerr << "QuantumVerse: Active quantum theory set to: CDT" << std::endl;
        std::cerr.flush();

        // Create the Camera4DAdapter for 4D navigation with 6-plane SO(4) rotation
        std::cerr << "QuantumVerse: Creating Camera4DAdapter..." << std::endl;
        std::cerr.flush();
        auto camera4DAdapter = std::make_shared<quantumverse::Camera4DAdapter>();
        camera4DAdapter->setAnimationSpeed(1.0);
        std::cerr << "QuantumVerse: Camera4DAdapter created" << std::endl;
        std::cerr.flush();

        // Initialize VR backend if enabled
#ifdef QUANTUMVERSE_USE_VR
        auto vrBackend = std::make_shared<quantumverse::vr::OpenXRBackend>();
        quantumverse::vr::VRConfig vrConfig;
        vrConfig.enabled = true;
        vrConfig.debugMode = false;
        vrBackend->setConfig(vrConfig);
        if (vrBackend->initialize("QuantumVerse")) {
            qDebug() << "QuantumVerse: VR backend initialized";
        } else {
            qDebug() << "QuantumVerse: VR backend initialization failed - continuing without VR";
        }

        // VR controller input → Camera4DAdapter wiring
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [vrBackend, camera4DAdapter](QObject* obj, const QUrl& objUrl) {
            if (!obj) return;
            QTimer* vrInputTimer = new QTimer(&engine);
            vrInputTimer->setInterval(16); // ~60Hz polling
            QObject::connect(vrInputTimer, &QTimer::timeout, [vrBackend, camera4DAdapter]() {
                if (!vrBackend->isActive() || !camera4DAdapter) return;

                quantumverse::vr::ControllerState left, right;
                if (vrBackend->getControllerState(left, right)) {
                    // Left thumbstick → XY/XZ rotations
                    const float deadzone = 0.15f;
                    if (std::abs(left.thumbstickAxis.x) > deadzone) {
                        camera4DAdapter->onMouseMoved(
                            camera4DAdapter->distance() * left.thumbstickAxis.x * 0.02f,
                            0.0f, 0, 0);
                    }
                    if (std::abs(left.thumbstickAxis.y) > deadzone) {
                        camera4DAdapter->onMouseMoved(
                            0.0f,
                            camera4DAdapter->distance() * left.thumbstickAxis.y * 0.02f,
                            0, 0);
                    }

                    // Right thumbstick → TY/TZ 4D rotations
                    if (std::abs(right.thumbstickAxis.x) > deadzone) {
                        camera4DAdapter->rotateInPlane(right.thumbstickAxis.x * 0.05);
                    }
                    if (std::abs(right.thumbstickAxis.y) > deadzone) {
                        camera4DAdapter->advanceSlice(right.thumbstickAxis.y * 0.1);
                    }

                    // Grip button → reset view
                    if (left.buttons[quantumverse::vr::ControllerState::ButtonGrip]) {
                        camera4DAdapter->reset();
                    }
                }
            });
            vrInputTimer->start();
        }, Qt::QueuedConnection);
#endif

        // Create the Planck Microscope widget for Planck-scale exploration
        std::cerr << "QuantumVerse: Creating PlanckMicroscope..." << std::endl;
        std::cerr.flush();
        auto planckMicroscope = new quantumverse::PlanckMicroscope(nullptr);
        std::cerr << "QuantumVerse: PlanckMicroscope created" << std::endl;
        std::cerr.flush();
        QWidget* planckContainer = QWidget::createWindowContainer(
            planckMicroscope->windowHandle(), nullptr);
        std::cerr << "QuantumVerse: planckContainer created" << std::endl;
        std::cerr.flush();
        planckContainer->setObjectName("planckContainer");
        planckContainer->setVisible(false);
        std::cerr << "QuantumVerse: PlanckMicroscope setup complete" << std::endl;
        std::cerr.flush();

        // Set up the QML engine
        QQmlApplicationEngine engine;

        // Add QML import paths
        // The qmldir file is in deploy/windows/qml/QuantumVerse/
        engine.addImportPath(QStringLiteral("qrc:/"));
        engine.addImportPath(QStringLiteral("."));
        engine.addImportPath(QStringLiteral("qml"));
        engine.addImportPath(QStringLiteral("F:/syyyy/src/qml"));
        engine.addImportPath(QStringLiteral("F:/qt/6.11.1/msvc2022_64/qml"));

        // Create QML camera controller for basic 3D navigation
        auto camController = new quantumverse::QmlCamController(&engine);

        // Expose C++ objects to QML
        QQmlContext* rootContext = engine.rootContext();
        rootContext->setContextProperty("curvatureRenderer",
            QVariant::fromValue(curvatureRenderer.get()));
        rootContext->setContextProperty("quantumRenderer",
            QVariant::fromValue(quantumRenderer.get()));
        rootContext->setContextProperty("sceneGraphModel",
            new quantumverse::SceneGraphModel(&engine));
        rootContext->setContextProperty("sceneGraphManager",
            QVariant::fromValue(sceneGraphManager.get()));
        rootContext->setContextProperty("ui4d",
            QVariant::fromValue(ui4d.get()));
        rootContext->setContextProperty("camera4DAdapter",
            QVariant::fromValue(camera4DAdapter.get()));
        // Shared meters -> viewport-units factor so QML camera focus lands
        // where the C++ renderer draws the bodies (see kViewportScale above).
        rootContext->setContextProperty("viewportScale",
            QVariant::fromValue(kViewportScale));
        rootContext->setContextProperty("camController",
            QVariant::fromValue(camController));
        rootContext->setContextProperty("discoveryPanelManager",
            QVariant::fromValue(discoveryPanelManager.get()));
#ifdef QUANTUMVERSE_USE_VR
        rootContext->setContextProperty("vrBackend",
            QVariant::fromValue(vrBackend.get()));
#endif

        // Correlations are exposed via DiscoveryPanelManager::correlationsList
        // and emitted through its built-in signals.

        // Expose the QML-facing findings list model, kept in sync with the
        // discovery panel manager via its findingsChanged signal.
        auto findingsModel = new quantumverse::FindingsModel(&engine);
        QObject::connect(discoveryPanelManager.get(),
            &quantumverse::DiscoveryPanelManager::findingsChanged,
            findingsModel, [findingsModel, discoveryPanelManager]() {
                findingsModel->setFindings(discoveryPanelManager->findings());
            });
        findingsModel->setFindings(discoveryPanelManager->findings());
        rootContext->setContextProperty("findingsModel", findingsModel);

        // Live multi-messenger alerts: LIGO/IceCube alerts flow into the same
        // FindingsModel so they appear in the QML Anomaly Feed alongside
        // discovery instrument findings.
        auto ligoAdapter = std::make_shared<quantumverse::LIGOAdapter>();
        auto iceCubeAdapter = std::make_shared<quantumverse::IceCubeAdapter>();
        ligoAdapter->setCallback([findingsModel, ligoAdapter](const quantumverse::GravitationalWaveAlert& alert) {
            (void)ligoAdapter;
            findingsModel->addFinding(quantumverse::toInstrumentFinding(alert));
        });
        iceCubeAdapter->setCallback([findingsModel, iceCubeAdapter](const quantumverse::NeutrinoAlert& alert) {
            (void)iceCubeAdapter;
            findingsModel->addFinding(quantumverse::toInstrumentFinding(alert));
        });
        ligoAdapter->start();
        iceCubeAdapter->start();
        rootContext->setContextProperty("ligoAdapter",
            QVariant::fromValue(ligoAdapter.get()));
        rootContext->setContextProperty("iceCubeAdapter",
            QVariant::fromValue(iceCubeAdapter.get()));

#ifdef HAVE_LIBRDKAFKA
        auto alertRouter = std::make_shared<quantumverse::AlertRouter>();
        alertRouter->setLIGOAdapter(ligoAdapter.get());
        alertRouter->setIceCubeAdapter(iceCubeAdapter.get());

        auto tessAdapter = std::make_shared<quantumverse::TESSAlertAdapter>();
        tessAdapter->setCallback([findingsModel](const quantumverse::TESSAlert& alert) {
            quantumverse::InstrumentFinding f;
            f.id = "TESS_" + alert.toi_id;
            f.instrumentName = "TESS (Live)";
            f.description = QStringLiteral("Live TOI %1 (P=%2 d, depth=%3 ppm)")
                .arg(QString::fromStdString(alert.toi_id))
                .arg(alert.period_days)
                .arg(alert.depth_ppm)
                .toStdString();
            f.confidence = alert.confidence;
            f.severity = quantumverse::confidenceToSeverity(alert.confidence);
            f.timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            findingsModel->addFinding(f);
        });
        alertRouter->setTESSAdapter(tessAdapter.get());
        rootContext->setContextProperty("tessAdapter",
            QVariant::fromValue(tessAdapter.get()));

        auto fermiAdapter = std::make_shared<quantumverse::FermiGBMAdapter>();
        fermiAdapter->setCallback([findingsModel](const quantumverse::FermiGBMAlert& alert) {
            quantumverse::InstrumentFinding f;
            f.id = "FermiGBM_" + alert.trigger_id;
            f.instrumentName = "Fermi GBM (Live)";
            f.description = QStringLiteral("Live GRB %1 (duration=%2 s, flux=%3)")
                .arg(QString::fromStdString(alert.trigger_id))
                .arg(alert.duration)
                .arg(alert.peak_flux)
                .toStdString();
            f.confidence = quantumverse::alertConfidence(alert.false_alarm_rate, alert.confidence);
            f.severity = quantumverse::confidenceToSeverity(f.confidence);
            f.timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            findingsModel->addFinding(f);
        });
        alertRouter->setFermiGBMAdapter(fermiAdapter.get());
        rootContext->setContextProperty("fermiAdapter",
            QVariant::fromValue(fermiAdapter.get()));

        auto kafkaListener = std::make_shared<quantumverse::KafkaAlertListener>(
            QStringLiteral("gcn-kafka.nasa.gov:9092"),
            {
                QStringLiteral("gcn.notices.LVC"),
                QStringLiteral("gcn.notices.ICECUBE"),
                QStringLiteral("gcn.notices.TESS"),
                QStringLiteral("gcn.notices.FERMI_GBM")
            }
        );

        QObject::connect(kafkaListener.get(), &quantumverse::KafkaAlertListener::alertReceived,
            alertRouter.get(), &quantumverse::AlertRouter::routeAlert);

        QObject::connect(alertRouter.get(), &quantumverse::AlertRouter::parsedAlertReady,
            discoveryPanelManager.get(), &quantumverse::DiscoveryPanelManager::ingestAlert);

        QObject::connect(kafkaListener.get(), &quantumverse::KafkaAlertListener::consumerError,
            [](const QString& err) {
                qWarning() << "Kafka listener error:" << err;
            });

        kafkaListener->start();
        rootContext->setContextProperty("kafkaListener",
            QVariant::fromValue(kafkaListener.get()));
        rootContext->setContextProperty("alertRouter",
            QVariant::fromValue(alertRouter.get()));

        auto replayStream = std::make_shared<quantumverse::GCNReplayStream>();
        QObject::connect(replayStream.get(), &quantumverse::GCNReplayStream::alertAvailable,
            alertRouter.get(), &quantumverse::AlertRouter::routeAlert);
        rootContext->setContextProperty("replayStream",
            QVariant::fromValue(replayStream.get()));
#endif
        rootContext->setContextProperty("planckContainer",
            QVariant::fromValue(planckContainer));

        // Stub missing properties so QML does not throw reference errors
        rootContext->setContextProperty("probeKretschmann",
            QVariant::fromValue(QString("—")));
        rootContext->setContextProperty("probeRicci",
            QVariant::fromValue(QString("—")));
        rootContext->setContextProperty("probeWeyl",
            QVariant::fromValue(QString("—")));
        rootContext->setContextProperty("probeRedshift",
            QVariant::fromValue(QString("—")));

        QStringList bodies = {"Schwarzschild BH"};
        engine.rootContext()->setContextProperty("celestialBodyModel",
            QVariant::fromValue(bodies));

        // Load the main QML file
        const QUrl url(QStringLiteral("qrc:/main.qml"));
        qDebug() << "QuantumVerse: Loading QML from" << url.toString();

        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
            &app, [url](QObject* obj, const QUrl& objUrl) {
                if (!obj && url == objUrl) {
                    std::cerr << "Failed to load main.qml from" << url.toString().toStdString() << std::endl;
                    std::cerr.flush();
                    QCoreApplication::exit(-1);
                }
            }, Qt::QueuedConnection);

        QQmlComponent component(&engine, url);
        if (component.isError()) {
            std::cerr << "QQmlComponent errors:" << std::endl;
            for (const QQmlError& error : component.errors()) {
                std::cerr << "  " << error.toString().toStdString() << std::endl;
            }
            std::cerr.flush();
            return -1;
        }

        engine.load(url);
        qDebug() << "QuantumVerse: QML engine.load() completed";

        std::ofstream("graphics_api.txt", std::ios::app) << "graphics_api_post_engine: " << QQuickWindow::graphicsApi() << std::endl;

        // Check for QML errors
        if (engine.rootObjects().isEmpty()) {
            qCritical() << "QML engine has no root objects - QML failed to load";
            return -1;
        }

        qDebug() << "QML loaded successfully, root objects:" << engine.rootObjects().size();

        // Initialize the scene graph model with the manager
        auto* sgModel = rootContext->contextProperty("sceneGraphModel")
            .value<quantumverse::SceneGraphModel*>();
        if (sgModel) {
            sgModel->setSceneGraphManager(sceneGraphManager);
            sgModel->createSolarSystem();
        }

        // Find the QmlGlViewport in the QML object tree and wire up renderers
        // This must happen after engine.load() so the QML object tree exists
        QObject* rootObject = engine.rootObjects().value(0, nullptr);
        quantumverse::QmlGlViewport* viewport = nullptr;
        if (rootObject) {
            // Find the viewport by object name set in QML
            viewport = rootObject->findChild<quantumverse::QmlGlViewport*>();
            if (viewport) {
                viewport->setCurvatureRendererDirect(curvatureRenderer);
                viewport->setProbeMetric(metric);
                viewport->setQuantumRendererDirect(quantumRenderer);
                viewport->setUI4D(ui4d);

                viewport->setCamera4DAdapterDirect(camera4DAdapter);

                auto celestialBodyRenderer = std::make_shared<quantumverse::CelestialBodyRenderer>(
                    quantumverse::CelestialBodyRenderer::QualityLevel::MEDIUM, 64);
                viewport->setCelestialBodyRendererDirect(celestialBodyRenderer);

                if (headlessFrames > 0) {
                    viewport->setHeadlessFrameTarget(headlessFrames);
                }

                if (enableGeodesics) {
                    qDebug() << "[DIAG] --enable-geodesics: forcing showGeodesics=true on viewport";
                    viewport->setShowGeodesics(true);
                    qDebug() << "[DIAG] --enable-geodesics: curvatureRenderer sync will happen in renderGL()";
                }

                if (!screenshotPath.isEmpty()) {
                    viewport->requestScreenshot(screenshotPath);
                }

                if (glStrict) {
                    quantumverse::GLDebug::instance().setStrictMode(true);
                }

                qDebug() << "QuantumVerse: Renderers, UI4D, Camera4DAdapter, and CelestialBodyRenderer wired to QML viewport";
                std::cerr << "QuantumVerse: Renderers, UI4D, Camera4DAdapter, and CelestialBodyRenderer wired to QML viewport" << std::endl;
                std::cerr.flush();
            } else {
                qWarning("QuantumVerse: Could not find QmlGlViewport in QML object tree");
                std::cerr << "QuantumVerse: Could not find QmlGlViewport in QML object tree" << std::endl;
                std::cerr.flush();
            }
        } else {
            std::cerr << "QuantumVerse: No root QML object found" << std::endl;
            std::cerr.flush();
        }

        // [DIAG] Auto-trigger a discovery scan a few seconds after startup so
        // the post-Scan render diagnostics can be captured without a manual
        // button click (use the --autoscan CLI flag).
        if (autoScan) {
            qDebug() << "Auto-scan enabled: will call startScan() after 2.5s";
            QTimer::singleShot(2500, &app, [discoveryPanelManager]() {
                qDebug() << "Auto-scan: invoking discoveryPanelManager->startScan()";
                discoveryPanelManager->startScan();
            });
        }

        qDebug() << "QuantumVerse Simulator started successfully";
        qDebug() << "  - OpenGL 4.5 Core Profile";
        qDebug() << "  - QML Scene Graph with" << sceneGraphManager->getScene().objects.size() << "objects";
        qDebug() << "  - Curvature renderer initialized";
        qDebug() << "  - Ready for 4D navigation";
        std::cerr << "QuantumVerse Simulator started successfully" << std::endl;
        std::cerr << "  - OpenGL 4.5 Core Profile" << std::endl;
        std::cerr << "  - QML Scene Graph with " << sceneGraphManager->getScene().objects.size() << " objects" << std::endl;
        std::cerr << "  - Curvature renderer initialized" << std::endl;
        std::cerr << "  - Ready for 4D navigation" << std::endl;
        std::cerr.flush();

        if (headlessFrames > 0) {
            qDebug() << "Headless benchmark mode: rendering" << headlessFrames << "frames";
            std::cerr << "Headless benchmark mode: rendering " << headlessFrames << " frames" << std::endl;
            std::cerr.flush();

            // Phase 1 warmup: complete GL initialization before benchmark.
            // This triggers one-time shader recompilation, context creation,
            // and driver warmup so measured frames reflect steady-state perf.
            std::cerr << "Headless warmup: rendering initial frames..." << std::endl;
            std::cerr.flush();
            for (int w = 0; w < 5; ++w) {
                if (viewport) {
                    viewport->update();
                }
                QCoreApplication::processEvents();
                QThread::msleep(50);
            }
            std::cerr << "Warmup complete, starting benchmark..." << std::endl;
            std::cerr.flush();

            QElapsedTimer elapsed;
            elapsed.start();
            qint64 lastTickNs = 0;
            int headlessRendered = 0;
            double headlessTotalMs = 0.0;
            double headlessMinMs = 1e9;
            double headlessMaxMs = 0.0;
            std::vector<double> frameDeltas;

            QTimer* headlessTimer = new QTimer(&app);
            headlessTimer->setInterval(16);
            QObject::connect(headlessTimer, &QTimer::timeout, [&]() {
                qint64 nowNs = elapsed.nsecsElapsed();
                double frameMs = 0.0;
                if (lastTickNs > 0) {
                    frameMs = (nowNs - lastTickNs) / 1e6;
                    if (frameMs < 0.5) frameMs = 0.5;

                    headlessTotalMs += frameMs;
                    if (frameMs < headlessMinMs) headlessMinMs = frameMs;
                    if (frameMs > headlessMaxMs) headlessMaxMs = frameMs;
                    frameDeltas.push_back(frameMs);
                }
                lastTickNs = nowNs;
                headlessRendered++;

                // Disable VR in headless mode
#ifdef QUANTUMVERSE_USE_VR
                if (viewport && viewport->vrActive()) {
                    viewport->setVrActive(false);
                }
#endif

                // [DIAG] Drive continuous rendering in headless mode. The QML
                // Timer that normally calls viewport.update() each frame is
                // paused when the window is not the active window, so without
                // this the render loop only runs a single frame.
                if (viewport) {
                    viewport->update();
                }

                if (frameDiagnostics.isEnabled()) {
                    quantumverse::utils::FrameSnapshot snap;
                    snap.frame_number = headlessRendered;
                    snap.timestamp_s = elapsed.nsecsElapsed() / 1e9;
                    snap.frame_time_ms = frameMs;
                    snap.physics_time_ms = 0.0;
                    snap.render_time_ms = 0.0;
                    snap.ui_time_ms = 0.0;
                    snap.active_geodesics = 0;
                    snap.vram_used_mb = 0.0;
                    snap.gl_error_count = quantumverse::GLDebug::instance().getErrorCount();
                    frameDiagnostics.recordFrame(snap);
                }

                if (headlessRendered >= headlessFrames) {
                    double avgMs = headlessTotalMs / headlessFrames;
                    double fps = avgMs > 0.0 ? 1000.0 / avgMs : 0.0;
                    qDebug() << "HeadlessPerformanceStats: frames=" << headlessRendered
                             << "avg=" << avgMs << "ms min=" << headlessMinMs << "ms max=" << headlessMaxMs
                             << "ms fps=" << fps;
                    std::cerr << "HeadlessPerformanceStats: frames=" << headlessRendered
                              << " avg=" << avgMs << "ms max=" << headlessMaxMs << "ms" << std::endl;
                    std::cerr.flush();

                    QFile file("headless_performance.log");
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&file);
                        out << "Average frame time: " << avgMs << " ms, Max: " << headlessMaxMs
                            << " ms, Min: " << headlessMinMs << " ms, FPS: " << fps
                            << " (frames=" << headlessRendered << ")\n";
                        file.close();
                    } else {
                        std::cerr << "Failed to open headless_performance.log" << std::endl;
                        std::cerr.flush();
                    }

                    if (!frameTimesPath.isEmpty()) {
                        QFile ftFile(frameTimesPath);
                        if (ftFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            QTextStream out(&ftFile);
                            for (double dt : frameDeltas) {
                                out << dt << "\n";
                            }
                            ftFile.close();
                        } else {
                            std::cerr << "Failed to open frame times file: "
                                      << frameTimesPath.toStdString() << std::endl;
                            std::cerr.flush();
                        }
                    }

                    headlessTimer->stop();
                    QTimer::singleShot(50, &app, &QApplication::quit);
                }
            });
            headlessTimer->start();
            QTimer::singleShot(headlessFrames * 100 + 5000, []() {
                std::cerr << "Headless benchmark safety timeout - exiting\n";
                std::cerr.flush();
                QCoreApplication::quit();
            });
        }

        // Run the application
        int result = app.exec();

        if (frameDiagnostics.isEnabled() && !diagnosticsPath.isEmpty()) {
            bool written = frameDiagnostics.writeJson(diagnosticsPath.toStdString());
            if (written) {
                std::cerr << "Frame diagnostics written to " << diagnosticsPath.toStdString() << std::endl;
            } else {
                std::cerr << "Failed to write frame diagnostics to " << diagnosticsPath.toStdString() << std::endl;
            }
            std::cerr.flush();
        }

        if (glStrict && quantumverse::GLDebug::instance().isEnabled()) {
            int errCount = quantumverse::GLDebug::instance().getErrorCount();
            if (errCount > 0) {
                std::cerr << "GL strict mode: " << errCount << " GL error(s) detected during rendering" << std::endl;
                std::cerr.flush();
                return 1;
            }
        }

        // Cleanup on exit
        std::cerr << "Shutting down QuantumVerse QML application" << std::endl;
        std::cerr.flush();
        // TRACE_INIT("Shutting down QuantumVerse QML application");
        // quantumverse::utils::TraceLogger::instance().shutdown();

        return result;
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION in main: " << e.what() << std::endl;
        std::cerr.flush();
        MessageBoxA(nullptr, e.what(), "QuantumVerse Fatal Error", MB_ICONERROR);
        return -1;
    } catch (...) {
        std::cerr << "FATAL UNKNOWN EXCEPTION in main" << std::endl;
        std::cerr.flush();
        MessageBoxA(nullptr, "Unknown fatal error", "QuantumVerse Fatal Error", MB_ICONERROR);
        return -1;
    }
}
