#include "WaveformView.h"

namespace librairy
{

WaveformView::WaveformView()
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);

    // Transport buttons styling
    auto setupBtn = [](juce::TextButton& btn, const juce::String& text)
    {
        btn.setButtonText(text);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2D2D2D));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    };

    setupBtn(rewindButton, "<< -5s");
    rewindButton.onClick = [this]() { if (onRewindClicked) onRewindClicked(); };
    addAndMakeVisible(rewindButton);

    setupBtn(playPauseButton, "Play");
    playPauseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2E66B2));
    playPauseButton.onClick = [this]() { if (onPlayPauseClicked) onPlayPauseClicked(); };
    addAndMakeVisible(playPauseButton);

    setupBtn(stopButton, "Stop");
    stopButton.onClick = [this]() { if (onStopClicked) onStopClicked(); };
    addAndMakeVisible(stopButton);

    setupBtn(forwardButton, "+5s >>");
    forwardButton.onClick = [this]() { if (onFastForwardClicked) onFastForwardClicked(); };
    addAndMakeVisible(forwardButton);

    // Time label
    timeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    timeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    timeLabel.setText("00:00.0 / 00:00.0", juce::dontSendNotification);
    addAndMakeVisible(timeLabel);

    // Gain / Volume
    gainLabel.setFont(juce::FontOptions(12.0f));
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(gainLabel);

    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    gainSlider.setRange(0.0, 2.0, 0.01);
    gainSlider.setValue(1.0, juce::dontSendNotification);
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF4CAF50));
    gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF333333));
    gainSlider.onValueChange = [this]()
    {
        if (onGainChanged)
            onGainChanged(static_cast<float>(gainSlider.getValue()));
    };
    addAndMakeVisible(gainSlider);
}

WaveformView::~WaveformView()
{
    thumbnail.removeChangeListener(this);
}

juce::String WaveformView::formatTime(double totalSeconds)
{
    if (totalSeconds < 0.0) totalSeconds = 0.0;
    int mins = static_cast<int>(totalSeconds) / 60;
    double secs = totalSeconds - (mins * 60);
    return juce::String::formatted("%02d:%04.1f", mins, secs);
}

void WaveformView::setPlaybackInfo(double currentSeconds, double totalDuration, bool isPlaying)
{
    playPauseButton.setButtonText(isPlaying ? "Pause" : "Play");
    playPauseButton.setColour(juce::TextButton::buttonColourId,
                              isPlaying ? juce::Colour(0xFFD32F2F) : juce::Colour(0xFF2E66B2));

    timeLabel.setText(formatTime(currentSeconds) + " / " + formatTime(totalDuration),
                      juce::dontSendNotification);
}

void WaveformView::setGainValue(float gain)
{
    gainSlider.setValue(gain, juce::dontSendNotification);
}

void WaveformView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF141414));

    // Top transport toolbar background
    auto barRect = getLocalBounds().removeFromTop(36);
    g.setColour(juce::Colour(0xFF1F1F1F));
    g.fillRect(barRect);

    g.setColour(juce::Colour(0xFF333333));
    g.drawLine(0.f, 36.f, static_cast<float>(getWidth()), 36.f);

    // Waveform area background
    g.setColour(juce::Colour(0xFF181818));
    g.fillRect(waveformArea);

    if (fileLoaded && thumbnail.getTotalLength() > 0.0)
    {
        // Draw waveform
        g.setColour(juce::Colour(0xFF4CAF50)); // Bright green
        thumbnail.drawChannels(g, waveformArea, 0.0,
                               thumbnail.getTotalLength(), 1.0f);

        // Draw playback cursor
        auto cursorX = static_cast<float>(waveformArea.getX() +
                                          currentPosition * waveformArea.getWidth());
        g.setColour(juce::Colours::white);
        g.drawLine(cursorX, static_cast<float>(waveformArea.getY()),
                   cursorX, static_cast<float>(waveformArea.getBottom()), 2.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("No audio selected", waveformArea,
                   juce::Justification::centred, true);
    }

    // Outer border
    g.setColour(juce::Colour(0xFF333333));
    g.drawRect(getLocalBounds(), 1);
}

void WaveformView::resized()
{
    auto bounds = getLocalBounds();

    // Top 36px for transport strip
    auto transportBar = bounds.removeFromTop(36).reduced(6, 4);

    rewindButton.setBounds(transportBar.removeFromLeft(60));
    transportBar.removeFromLeft(4);
    playPauseButton.setBounds(transportBar.removeFromLeft(55));
    transportBar.removeFromLeft(4);
    stopButton.setBounds(transportBar.removeFromLeft(50));
    transportBar.removeFromLeft(4);
    forwardButton.setBounds(transportBar.removeFromLeft(60));
    transportBar.removeFromLeft(12);

    timeLabel.setBounds(transportBar.removeFromLeft(160));

    // Gain slider on right side
    gainSlider.setBounds(transportBar.removeFromRight(150));
    gainLabel.setBounds(transportBar.removeFromRight(40));

    // Remaining area is waveform
    waveformArea = bounds.reduced(2, 2);
}

void WaveformView::loadFile(const juce::File& file)
{
    thumbnail.clear();
    fileLoaded = false;
    currentPosition = 0.0;

    if (file.existsAsFile())
    {
        thumbnail.setSource(new juce::FileInputSource(file));
        fileLoaded = true;
    }

    repaint();
}

void WaveformView::clear()
{
    thumbnail.clear();
    fileLoaded = false;
    currentPosition = 0.0;
    timeLabel.setText("00:00.0 / 00:00.0", juce::dontSendNotification);
    repaint();
}

void WaveformView::setPlaybackPosition(double normalizedPosition)
{
    currentPosition = juce::jlimit(0.0, 1.0, normalizedPosition);
    repaint();
}

void WaveformView::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    repaint();
}

void WaveformView::mouseDown(const juce::MouseEvent& event)
{
    if (waveformArea.contains(event.getPosition()))
        updatePositionFromMouse(event);
}

void WaveformView::mouseDrag(const juce::MouseEvent& event)
{
    if (waveformArea.contains(event.getPosition()))
        updatePositionFromMouse(event);
}

void WaveformView::updatePositionFromMouse(const juce::MouseEvent& event)
{
    if (!fileLoaded || waveformArea.getWidth() <= 0)
        return;

    double pos = static_cast<double>(event.x - waveformArea.getX()) /
                 static_cast<double>(waveformArea.getWidth());
    pos = juce::jlimit(0.0, 1.0, pos);

    currentPosition = pos;
    listeners.call([pos](Listener& l) { l.positionClicked(pos); });
    repaint();
}

void WaveformView::addListener(Listener* listener)    { listeners.add(listener); }
void WaveformView::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy
