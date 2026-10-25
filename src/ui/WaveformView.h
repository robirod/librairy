#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace librairy
{

/**
 * Waveform display with playback cursor, zoom, and scrub support.
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

    // ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Mouse interaction
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

    juce::ListenerList<Listener> listeners;

    void updatePositionFromMouse(const juce::MouseEvent& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace librairy

