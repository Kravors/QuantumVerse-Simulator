/**
 * @file GravitationalWaveAudio.h
 * @brief Real-time audio synthesizer for gravitational wave strain
 *
 * Generates a sine wave tone whose frequency and amplitude track
 * the gravitational wave strain from the N-body simulation.
 */

#ifndef QUANTUMVERSE_GW_AUDIO_H
#define QUANTUMVERSE_GW_AUDIO_H

#include <QObject>
#include <QIODevice>
#include <QTimer>
#include <cmath>
#include <atomic>

#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
#include <QAudioSink>
#include <QMediaDevices>
#endif

namespace quantumverse {

class GravitationalWaveAudio : public QObject {
    Q_OBJECT

public:
    explicit GravitationalWaveAudio(QObject* parent = nullptr);
    ~GravitationalWaveAudio() override;

    void start();
    void stop();
    bool isPlaying() const { return playing_; }

    // Set strain (0-1 normalized) and frequency (Hz)
    void setStrain(double strain);
    void setFrequency(double freq);
    void setVolume(double volume) { volume_.store(std::clamp(volume, 0.0, 1.0)); }
    
    // Waveform mode
    enum class WaveformMode {
        Sine,           // Simple sine wave (original)
        Chirp,          // Inspiral chirp (frequency increases with amplitude)
        InspiralMergerRingdown  // Full IMR waveform
    };
    void setWaveformMode(WaveformMode mode) { waveformMode_ = mode; }
    void setMergerPhase(double phase) { mergerPhase_.store(phase); } // 0=early, 1=merger

signals:
    void audioDataUpdated(double amplitude);

private slots:
    void generateAudio();

private:
#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
    QAudioSink* audioSink_ = nullptr;
#endif
    QIODevice* audioDevice_ = nullptr;
    QTimer* timer_ = nullptr;

    std::atomic<double> strain_{0.0};
    std::atomic<double> frequency_{100.0};
    std::atomic<double> volume_{0.5};
    std::atomic<double> phase_{0.0};
    bool playing_ = false;

    // Spatial audio state
    std::atomic<double> pan_{0.0};
    std::atomic<double> srcX_{0.0}, srcY_{0.0}, srcZ_{0.0};
    std::atomic<double> listenerX_{0.0}, listenerY_{0.0}, listenerZ_{0.0};
    std::atomic<double> fwdX_{0.0}, fwdY_{0.0}, fwdZ_{1.0};
    
    // Waveform state
    WaveformMode waveformMode_ = WaveformMode::Chirp;
    std::atomic<double> mergerPhase_{0.0};
    double chirpPhase_ = 0.0;

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2; // Stereo for spatial audio
    static constexpr int BUFFER_SIZE = 4096;

public:
    // Spatial audio parameters (public for QmlGlViewport access)
    void setPan(double pan) { pan_.store(std::clamp(pan, -1.0, 1.0)); }
    void setSourcePosition(double x, double y, double z) { srcX_.store(x); srcY_.store(y); srcZ_.store(z); }
    void setListenerPosition(double x, double y, double z) { listenerX_.store(x); listenerY_.store(y); listenerZ_.store(z); }
    void setListenerForward(double fx, double fy, double fz) { fwdX_.store(fx); fwdY_.store(fy); fwdZ_.store(fz); }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GW_AUDIO_H
