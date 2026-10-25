#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace librairy
{

/**
 * Audio playback engine for previewing audio files.
 *
 * Provides low-latency playback with transport controls
 * (play, stop, pause, seek) and position reporting.
 */
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    /** Load a file for playback */
    bool loadFile(const juce::File& file);

    /** Unload the current file */
    void unloadFile();

    /** Transport controls */
    void play();
    void stop();
    void pause();
    void togglePlayPause();

    /** Seek to normalized position (0.0 - 1.0) */
    void seekTo(double normalizedPosition);

    /** Get current playback position (0.0 - 1.0) */
    double getPosition() const;

    /** Get current playback position in seconds */
    double getPositionSeconds() const;

    /** Is currently playing? */
    bool isPlaying() const;

    /** Has a file loaded? */
    bool isFileLoaded() const;

    /** Get total duration in seconds */
    double getDuration() const;

    /** Set playback volume (0.0 - 2.0, default 1.0) */
    void setVolume(float volume);

    /** Get current playback volume */
    float getVolume() const { return currentVolume; }

    /** Seek backward / rewind by seconds */
    void rewind(double seconds = 5.0);

    /** Seek forward by seconds */
    void fastForward(double seconds = 5.0);

    /** Set the audio output device */
    void setOutputDevice(const juce::String& deviceName);

    /** Get available output devices */
    juce::StringArray getAvailableDevices() const;

    /** Get the currently selected output device name */
    juce::String getCurrentOutputDeviceName() const;

    /** Expose AudioDeviceManager for AudioDeviceSelectorComponent */
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    /** Listener for position updates */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void playbackPositionChanged(double normalizedPos) = 0;
        virtual void playbackStarted() = 0;
        virtual void playbackStopped() = 0;
        virtual void fileLoaded(const juce::File& file) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    juce::AudioDeviceManager     deviceManager;
    juce::AudioFormatManager     formatManager;
    juce::AudioSourcePlayer      sourcePlayer;
    juce::AudioTransportSource   transportSource;

    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    juce::File currentFile;
    float      currentVolume = 1.0f;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace librairy

