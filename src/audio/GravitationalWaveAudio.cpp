/**
 * @file GravitationalWaveAudio.cpp
 * @brief Real-time audio synthesizer for gravitational wave strain
 */

#include "audio/GravitationalWaveAudio.h"
#include <QDebug>
#include <cstring>

#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
#include <QAudioFormat>
#include <QMediaDevices>
#endif

namespace quantumverse {

GravitationalWaveAudio::GravitationalWaveAudio(QObject* parent)
    : QObject(parent)
{
    timer_ = new QTimer(this);
    timer_->setInterval(20); // 50 Hz update rate
    connect(timer_, &QTimer::timeout, this, &GravitationalWaveAudio::generateAudio);
}

GravitationalWaveAudio::~GravitationalWaveAudio()
{
    stop();
}

void GravitationalWaveAudio::start()
{
#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
    if (playing_) return;

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    auto device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "[GW-Audio] Format not supported, using nearest";
        format = device.preferredFormat();
    }

    audioSink_ = new QAudioSink(device, format, this);
    audioSink_->setBufferSize(BUFFER_SIZE);
    audioDevice_ = audioSink_->start();

    playing_ = true;
    timer_->start();

    qDebug() << "[GW-Audio] Started audio output";
#else
    qWarning() << "[GW-Audio] Multimedia support not compiled in";
#endif
}

void GravitationalWaveAudio::stop()
{
#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
    if (!playing_) return;

    timer_->stop();
    if (audioSink_) {
        audioSink_->stop();
        delete audioSink_;
        audioSink_ = nullptr;
    }
    audioDevice_ = nullptr;
    playing_ = false;

    qDebug() << "[GW-Audio] Stopped audio output";
#else
    playing_ = false;
#endif
}

void GravitationalWaveAudio::setStrain(double strain)
{
    strain_.store(std::clamp(strain, 0.0, 1.0));
}

void GravitationalWaveAudio::setFrequency(double freq)
{
    frequency_.store(std::clamp(freq, 20.0, 2000.0)); // Audible range
}

void GravitationalWaveAudio::generateAudio()
{
#ifdef QUANTUMVERSE_HAS_MULTIMEDIA
    if (!audioDevice_ || !audioSink_) return;

    double strain = strain_.load();
    double freq = frequency_.load();
    double pan = pan_.load();

    // Map strain to amplitude (log scale for better perception)
    double amplitude = strain * volume_.load() * 0.5; // Max 50% volume

    // Distance attenuation (inverse square law with clamp)
    double dx = srcX_.load() - listenerX_.load();
    double dy = srcY_.load() - listenerY_.load();
    double dz = srcZ_.load() - listenerZ_.load();
    double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    double attenuation = 1.0 / (1.0 + dist * 1e-10); // Gentle falloff
    
    // Calculate stereo pan based on direction relative to listener forward
    double leftGain = 0.5, rightGain = 0.5;
    if (dist > 1e-6) {
        double fwdLen = std::sqrt(fwdX_.load() * fwdX_.load() + fwdY_.load() * fwdY_.load() + fwdZ_.load() * fwdZ_.load());
        double fwdX = fwdX_.load() / fwdLen;
        double fwdY = fwdY_.load() / fwdLen;
        double fwdZ = fwdZ_.load() / fwdLen;
        
        double dirX = dx / dist;
        double dirY = dy / dist;
        double dirZ = dz / dist;
        
        double crossX = fwdY * dirZ - fwdZ * dirY;
        double crossY = fwdZ * dirX - fwdX * dirZ;
        
        double sideComponent = std::sqrt(crossX * crossX + crossY * crossY);
        double panFromDirection = std::max(-1.0, std::min(1.0, sideComponent * (crossY >= 0 ? 1.0 : -1.0)));
        double combinedPan = std::max(-1.0, std::min(1.0, pan + panFromDirection * 0.7));
        
        leftGain = std::cos((combinedPan + 1.0) * M_PI / 4.0);
        rightGain = std::sin((combinedPan + 1.0) * M_PI / 4.0);
    }
    
    leftGain *= attenuation;
    rightGain *= attenuation;

    // Generate stereo waveform samples
    int numSamples = BUFFER_SIZE / (sizeof(int16_t) * CHANNELS);
    QByteArray buffer(numSamples * sizeof(int16_t) * CHANNELS, 0);
    int16_t* samples = reinterpret_cast<int16_t*>(buffer.data());

    double phase = phase_.load();
    double phaseIncrement = 2.0 * M_PI * freq / SAMPLE_RATE;
    double mergerPhase = mergerPhase_.load();

    for (int i = 0; i < numSamples; ++i) {
        double sample = 0.0;
        
        switch (waveformMode_) {
            case WaveformMode::Sine:
                sample = amplitude * std::sin(phase);
                break;
                
            case WaveformMode::Chirp: {
                // Chirp: frequency increases with strain (inspiral)
                // Higher strain = higher frequency = closer to merger
                double chirpFreq = freq * (1.0 + strain * 10.0);
                double chirpIncrement = 2.0 * M_PI * chirpFreq / SAMPLE_RATE;
                sample = amplitude * std::sin(chirpPhase_);
                chirpPhase_ += chirpIncrement;
                if (chirpPhase_ > 2.0 * M_PI) chirpPhase_ -= 2.0 * M_PI;
                break;
            }
                
            case WaveformMode::InspiralMergerRingdown: {
                // Full IMR waveform
                if (mergerPhase < 0.8) {
                    // Inspiral: chirp with increasing frequency
                    double inspiralFreq = freq * (1.0 + strain * 10.0 * (mergerPhase / 0.8));
                    double inspiralAmp = amplitude * (0.2 + 0.8 * (mergerPhase / 0.8));
                    double inspIncrement = 2.0 * M_PI * inspiralFreq / SAMPLE_RATE;
                    sample = inspiralAmp * std::sin(chirpPhase_);
                    chirpPhase_ += inspIncrement;
                } else if (mergerPhase < 0.95) {
                    // Merger: peak amplitude, rapid oscillation
                    double mergerFreq = freq * 20.0;
                    double mergerAmp = amplitude * 1.5;
                    double mergerIncrement = 2.0 * M_PI * mergerFreq / SAMPLE_RATE;
                    sample = mergerAmp * std::sin(chirpPhase_);
                    chirpPhase_ += mergerIncrement;
                } else {
                    // Ringdown: exponential decay
                    double ringdownFreq = freq * 15.0;
                    double ringdownAmp = amplitude * std::exp(-10.0 * (mergerPhase - 0.95));
                    double ringdownIncrement = 2.0 * M_PI * ringdownFreq / SAMPLE_RATE;
                    sample = ringdownAmp * std::sin(chirpPhase_);
                    chirpPhase_ += ringdownIncrement;
                }
                if (chirpPhase_ > 2.0 * M_PI) chirpPhase_ -= 2.0 * M_PI;
                break;
            }
        }
        
        // If not using chirp modes, advance the regular phase
        if (waveformMode_ == WaveformMode::Sine) {
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        samples[i * 2 + 0] = static_cast<int16_t>(sample * leftGain * 32767.0);  // Left
        samples[i * 2 + 1] = static_cast<int16_t>(sample * rightGain * 32767.0); // Right
    }

    phase_.store(phase);

    audioDevice_->write(buffer);
    emit audioDataUpdated(amplitude);
#else
    emit audioDataUpdated(0.0);
#endif
}

} // namespace quantumverse
