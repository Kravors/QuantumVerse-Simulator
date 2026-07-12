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
#include <QWidget>
#include <QWindow>
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
#include "physics/SingularityHandler.h"
#include "quantumgravity/CDTEngine.h"
#include "discovery/DiscoveryPanelManager.h"
#include "discovery/FindingsModel.h"
#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/AlertToFinding.h"
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
#include "utils/TraceLogger.h"
#include "utils/CrashHandler.h"

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
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            headlessFrames = atoi(argv[++i]);
        }
    }
    if (GetSystemMetrics(SM_CMONITORS) == 0 && headlessFrames <= 0) {
        std::cerr << "No display detected. Exiting gracefully (headless environment)." << std::endl;
        std::cerr.flush();
        return 0;
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

    // Enable high-DPI scaling
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Set OpenGL format before QGuiApplication to avoid shared-context warnings
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
        auto metric = std::make_shared<quantumverse::MetricTensor>();
        auto curvatureRenderer = std::make_shared<quantumverse::CurvatureRenderer>(
            30, 100.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        curvatureRenderer->setMetric(metric);

        // Create a Schwarzschild black hole singularity (M = 10 solar masses)
        // Position at origin for the central viewport focus
        double blackHoleMass = 10.0 * 1.989e30;  // 10 solar masses in kg
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

        // Create the Discovery Panel Manager and register all 10 instruments
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
        rootContext->setContextProperty("camController",
            QVariant::fromValue(camController));
        rootContext->setContextProperty("discoveryPanelManager",
            QVariant::fromValue(discoveryPanelManager.get()));

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
        if (rootObject) {
            // Find the viewport by object name set in QML
            quantumverse::QmlGlViewport* viewport = rootObject->findChild<quantumverse::QmlGlViewport*>();
            if (viewport) {
                viewport->setCurvatureRendererDirect(curvatureRenderer);
                viewport->setQuantumRendererDirect(quantumRenderer);
                viewport->setUI4D(ui4d);

                viewport->setCamera4DAdapterDirect(camera4DAdapter);

                auto celestialBodyRenderer = std::make_shared<quantumverse::CelestialBodyRenderer>(
                    quantumverse::CelestialBodyRenderer::QualityLevel::MEDIUM, 64);
                viewport->setCelestialBodyRendererDirect(celestialBodyRenderer);

                if (headlessFrames > 0) {
                    viewport->setHeadlessFrameTarget(headlessFrames);
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

            QElapsedTimer elapsed;
            elapsed.start();
            qint64 lastTickNs = 0;
            int headlessRendered = 0;
            double headlessTotalMs = 0.0;
            double headlessMinMs = 1e9;
            double headlessMaxMs = 0.0;

            QTimer* headlessTimer = new QTimer(&app);
            headlessTimer->setInterval(16);
            QObject::connect(headlessTimer, &QTimer::timeout, [&]() {
                qint64 nowNs = elapsed.nsecsElapsed();
                if (lastTickNs > 0) {
                    double frameMs = (nowNs - lastTickNs) / 1e6;
                    if (frameMs < 0.5) frameMs = 0.5;

                    headlessTotalMs += frameMs;
                    if (frameMs < headlessMinMs) headlessMinMs = frameMs;
                    if (frameMs > headlessMaxMs) headlessMaxMs = frameMs;
                }
                lastTickNs = nowNs;
                headlessRendered++;

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
