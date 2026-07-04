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

// Qt headers MUST come before project headers to avoid namespace pollution.
// Qt uses QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros internally.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QResource>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <iostream>

#include "qmlglviewport.h"
#include "qmlcamcontroller.h"
#include "scenegraphmodel.h"
#include "ui4d/Camera4DAdapter.h"
#include "ui4d/SceneGraphManager.h"
#include "ui4d/UI4D.h"
#include "ui4d/Camera4DAdapter.h"
#include "spacetime/MetricTensor.h"
#include "rendering/CurvatureRenderer.h"
#include "rendering/QuantumGeometryRenderer.h"
#include "rendering/CelestialBodyRenderer.h"
#include "physics/SingularityHandler.h"
#include "quantumgravity/CDTEngine.h"
#include "discovery/DiscoveryPanelManager.h"
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
    if (GetSystemMetrics(SM_CMONITORS) == 0) {
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

        // Enable high-DPI scaling
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

        // Set OpenGL format before QGuiApplication to avoid shared-context warnings
        QSurfaceFormat::setDefaultFormat(createSurfaceFormat());

        QGuiApplication app(argc, argv);
        app.setApplicationName("QuantumVerse Simulator");
        app.setOrganizationName("QuantumVerse");

        qDebug() << "QT_OPENGL env:" << qgetenv("QT_OPENGL").constData();
        qDebug() << "OpenGL API before engine load:" << QQuickWindow::graphicsApi();

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
        qDebug() << "QuantumVerse: Setting active quantum theory to CDT";
        ui4d->setActiveQuantumTheory("CDT");
        qDebug() << "QuantumVerse: Active quantum theory set to: CDT";

        // Create the Camera4DAdapter for 4D navigation with 6-plane SO(4) rotation
        auto camera4DAdapter = std::make_shared<quantumverse::Camera4DAdapter>();
        camera4DAdapter->setAnimationSpeed(1.0);

        // Set up the QML engine
        QQmlApplicationEngine engine;

        // Add QML import paths
        // The qmldir file is in deploy/windows/qml/QuantumVerse/
        engine.addImportPath(QStringLiteral("qrc:/"));
        // Also add the current directory for file system QML modules
        engine.addImportPath(QStringLiteral("."));
        // Add the qml subdirectory for QuantumVerse module
        engine.addImportPath(QStringLiteral("qml"));
        // Add Qt's own QML module directory so QtQuick/QtQuick.Controls resolve from resources
        engine.addImportPath(QStringLiteral("F:/syyyy/Qt6/6.5.3/msvc2019_64/qml"));

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

                // Wire Camera4DAdapter to the viewport
                viewport->setCamera4DAdapterDirect(camera4DAdapter);

                // Initialize celestial body renderer in the viewport's renderer
                // (deferred to render() when GL context is available)
                // Bodies will be populated in renderGeodesics() from UI4D solar system data
                auto celestialBodyRenderer = std::make_shared<quantumverse::CelestialBodyRenderer>(
                    quantumverse::CelestialBodyRenderer::QualityLevel::MEDIUM, 64);
                viewport->setCelestialBodyRendererDirect(celestialBodyRenderer);

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
