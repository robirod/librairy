#include "WaveformView.h"

namespace librairy
{

WaveformView::WaveformView()
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);
}

WaveformView::~WaveformView()
{
    thumbnail.removeChangeListener(this);
}

void WaveformView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF181818));

    auto bounds = getLocalBounds();

    if (fileLoaded && thumbnail.getTotalLength() > 0.0)
    {
        // Draw waveform
        g.setColour(juce::Colour(0xFF4CAF50)); // Green waveform
        thumbnail.drawChannels(g, bounds, 0.0,
                               thumbnail.getTotalLength(), 1.0f);

        // Draw playback cursor
        g.setColour(juce::Colours::white);
        auto cursorX = static_cast<float>(currentPosition * bounds.getWidth());
        g.drawLine(cursorX, 0.f, cursorX,
                   static_cast<float>(bounds.getHeight()), 2.0f);
    }
    else
    {
        // Placeholder
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("No file loaded", bounds,
                   juce::Justification::centred, true);
    }

    // Top border
    g.setColour(juce::Colour(0xFF3D3D3D));
    g.drawLine(0.f, 0.f,
               static_cast<float>(getWidth()), 0.f);
}

void WaveformView::resized()
{
    // Nothing special needed — paint handles everything
}

void WaveformView::loadFile(const juce::File& file)
{
    thumbnail.clear();
    fileLoaded = false;
    currentPosition = 0.0;

    if (file.existsAsFile())
    {
        auto* reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            auto source = std::make_unique<juce::AudioFormatReaderSource>(
                reader, true);
            thumbnail.setSource(
                new juce::FileInputSource(file));
            fileLoaded = true;
        }
    }

    repaint();
}

void WaveformView::clear()
{
    thumbnail.clear();
    fileLoaded = false;
    currentPosition = 0.0;
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
    updatePositionFromMouse(event);
}

void WaveformView::mouseDrag(const juce::MouseEvent& event)
{
    updatePositionFromMouse(event);
}

void WaveformView::updatePositionFromMouse(const juce::MouseEvent& event)
{
    if (!fileLoaded)
        return;

    double pos = static_cast<double>(event.x) / static_cast<double>(getWidth());
    pos = juce::jlimit(0.0, 1.0, pos);

    currentPosition = pos;
    listeners.call([pos](Listener& l) { l.positionClicked(pos); });
    repaint();
}

void WaveformView::addListener(Listener* listener)    { listeners.add(listener); }
void WaveformView::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy

