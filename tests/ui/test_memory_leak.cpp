// Phase 5.5: Memory Leak Detection Test
// Runs the headless quantumverse_qml binary under ASan and asserts zero leaks.
// Only active when QUANTUMVERSE_USE_ASAN is enabled at build time.

#include <QTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

class MemoryLeakTest : public QObject
{
    Q_OBJECT

private slots:
    void test_headless_no_leaks()
    {
#ifdef __SANITIZE_ADDRESS__
        QString exePath = QCoreApplication::applicationDirPath() + QLatin1String("/quantumverse_qml.exe");
        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1");
        proc.setProcessEnvironment(env);
        proc.start(exePath, {"--headless", "--frames", "50", "--gl-strict"});
        bool finished = proc.waitForFinished(60000);
        QVERIFY2(finished, "quantumverse_qml did not finish within timeout");
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(proc.exitCode(), 0);
#else
        QSKIP("This test requires an ASan build (__SANITIZE_ADDRESS__ not defined)");
#endif
    }
};

QTEST_MAIN(MemoryLeakTest)
#include "test_memory_leak.moc"
