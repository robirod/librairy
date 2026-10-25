#include "AudioEngine.h"

namespace librairy
{

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();

    // Initialize audio device with default settings
    auto result = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (result.isNotEmpty())
        DBG("AudioEngine: Device init error: " << result);

    deviceManager.addAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(&transportSource);
}

AudioEngine::~AudioEngine()
{
    transportSource.setSource(nullptr);
    sourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&sourcePlayer);
}

bool AudioEngine::loadFile(const juce::File& file)
{
    unloadFile();

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(
        reader, true);

    transportSource.setSource(
        readerSource.get(), 0, nullptr,
        reader->sampleRate, static_cast<int>(reader->numChannels));

    transportSource.setGain(currentVolume);
    currentFile = file;

    listeners.call([file](Listener& l) { l.fileLoaded(file); });
    return true;
}

void AudioEngine::unloadFile()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();
    currentFile = juce::File();
}

void AudioEngine::play()
{
    if (readerSource != nullptr)
    {
        transportSource.start();
        listeners.call([](Listener& l) { l.playbackStarted(); });
    }
}

void AudioEngine::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
    listeners.call([](Listener& l) { l.playbackStopped(); });
}

void AudioEngine::pause()
{
    transportSource.stop();
    listeners.call([](Listener& l) { l.playbackStopped(); });
}

void AudioEngine::togglePlayPause()
{
    if (transportSource.isPlaying())
        pause();
    else
        play();
}

void AudioEngine::seekTo(double normalizedPosition)
{
    auto duration = transportSource.getLengthInSeconds();
    transportSource.setPosition(normalizedPosition * duration);
}

double AudioEngine::getPosition() const
{
    auto duration = transportSource.getLengthInSeconds();
    if (duration <= 0.0)
        return 0.0;
    return transportSource.getCurrentPosition() / duration;
}

double AudioEngine::getPositionSeconds() const
{
    return transportSource.getCurrentPosition();
}

bool AudioEngine::isPlaying() const
{
    return transportSource.isPlaying();
}

bool AudioEngine::isFileLoaded() const
{
    return readerSource != nullptr;
}

double AudioEngine::getDuration() const
{
    return transportSource.getLengthInSeconds();
}

void AudioEngine::setVolume(float volume)
{
    currentVolume = juce::jlimit(0.0f, 2.0f, volume);
    transportSource.setGain(currentVolume);
}

void AudioEngine::rewind(double seconds)
{
    double newPos = juce::jmax(0.0, getPositionSeconds() - seconds);
    transportSource.setPosition(newPos);
}

void AudioEngine::fastForward(double seconds)
{
    double newPos = juce::jmin(getDuration(), getPositionSeconds() + seconds);
    transportSource.setPosition(newPos);
}

void AudioEngine::setOutputDevice(const juce::String& deviceName)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.outputDeviceName = deviceName;
    deviceManager.setAudioDeviceSetup(setup, true);
}

juce::String AudioEngine::getCurrentOutputDeviceName() const
{
    return deviceManager.getAudioDeviceSetup().outputDeviceName;
}

juce::StringArray AudioEngine::getAvailableDevices() const
{
    juce::StringArray devices;
    if (auto* deviceType = deviceManager.getCurrentDeviceTypeObject())
        devices = deviceType->getDeviceNames(false);
    return devices;
}

void AudioEngine::addListener(Listener* listener)    { listeners.add(listener); }
void AudioEngine::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy

