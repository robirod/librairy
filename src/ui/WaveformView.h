#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>

namespace librairy
{

/**
 * Waveform display with integrated transport controls (Play, Pause, Stop, Rewind, Forward)
 * gain slider, time display, and interactive scrub.
 */
class WaveformView : public juce::Component,
                     public juce::ChangeListener
{
public:
    WaveformView();
    ~WaveformView() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Load a file and generate its waveform thumbnail */
    void loadFile(const juce::File& file);

    /** Clear the current waveform */
    void clear();

    /** Set the playback position (0.0 to 1.0) */
    void setPlaybackPosition(double normalizedPosition);

    /** Update time and playback status for transport buttons */
    void setPlaybackInfo(double currentSeconds, double totalDuration, bool isPlaying);

    /** Set volume gain slider value */
    void setGainValue(float gain);

    // Callbacks for transport
    std::function<void()>      onPlayPauseClicked;
    std::function<void()>      onStopClicked;
    std::function<void()>      onRewindClicked;
    std::function<void()>      onFastForwardClicked;
    std::function<void(float)> onGainChanged;

    // ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Mouse interaction on waveform
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    /** Listener for waveform interaction events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void positionClicked(double normalizedPosition) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    juce::AudioFormatManager       formatManager;
    juce::AudioThumbnailCache      thumbnailCache { 5 };
    juce::AudioThumbnail           thumbnail { 512, formatManager,
                                               thumbnailCache };

    double currentPosition = 0.0;
    bool   fileLoaded = false;

    // Transport UI
    juce::TextButton rewindButton    { "<<" };
    juce::TextButton playPauseButton { "Play" };
    juce::TextButton stopButton      { "Stop" };
    juce::TextButton forwardButton   { ">>" };
    juce::Label      timeLabel;
    juce::Label      gainLabel       { {}, "Gain:" };
    juce::Slider     gainSlider;

    juce::Rectangle<int> waveformArea;

    juce::ListenerList<Listener> listeners;

    void updatePositionFromMouse(const juce::MouseEvent& event);
    static juce::String formatTime(double seconds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace librairy
