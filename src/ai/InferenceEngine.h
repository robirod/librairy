#pragma once

/**
 * AI Inference Engine — Stage 2 stub.
 *
 * Will wrap ONNX Runtime to run trained audio classification
 * and embedding models at inference time.
 *
 * Only compiled when LIBRAIRY_ENABLE_AI is ON.
 */

#ifdef LIBRAIRY_AI_ENABLED

#include <juce_core/juce_core.h>
#include <string>
#include <vector>

namespace librairy
{

class InferenceEngine
{
public:
    InferenceEngine();
    ~InferenceEngine();

    /** Load an ONNX model from disk */
    bool loadModel(const juce::String& onnxPath);

    /** Classify an audio file — returns top-K category predictions */
    struct Prediction
    {
        juce::String category;
        float        confidence;
    };

    std::vector<Prediction> classify(const juce::File& audioFile,
                                     int topK = 5) const;

    /** Generate embedding vector for an audio file */
    std::vector<float> getEmbedding(const juce::File& audioFile) const;

    /** Is a model loaded? */
    bool isModelLoaded() const;

private:
    // ONNX Runtime session will live here
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace librairy

#endif // LIBRAIRY_AI_ENABLED

