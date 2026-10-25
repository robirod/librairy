#ifdef LIBRAIRY_AI_ENABLED

#include "InferenceEngine.h"

namespace librairy
{

struct InferenceEngine::Impl
{
    // TODO: ONNX Runtime session, environment, allocator
    bool modelLoaded = false;
};

InferenceEngine::InferenceEngine()
    : pImpl(std::make_unique<Impl>())
{
}

InferenceEngine::~InferenceEngine() = default;

bool InferenceEngine::loadModel(const juce::String& /*onnxPath*/)
{
    // TODO: Initialize ONNX Runtime session
    return false;
}

std::vector<InferenceEngine::Prediction> InferenceEngine::classify(
    const juce::File& /*audioFile*/, int /*topK*/) const
{
    // TODO: Preprocess audio → mel spectrogram → run inference
    return {};
}

std::vector<float> InferenceEngine::getEmbedding(
    const juce::File& /*audioFile*/) const
{
    // TODO: Run embedding model
    return {};
}

bool InferenceEngine::isModelLoaded() const
{
    return pImpl->modelLoaded;
}

} // namespace librairy

#endif // LIBRAIRY_AI_ENABLED

