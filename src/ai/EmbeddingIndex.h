#pragma once

/**
 * Embedding Index — Stage 2 stub.
 *
 * Will use FAISS or Annoy for fast nearest-neighbor search
 * over audio embeddings.
 */

#ifdef LIBRAIRY_AI_ENABLED

namespace librairy
{

class EmbeddingIndex
{
public:
    EmbeddingIndex() = default;
    ~EmbeddingIndex() = default;

    // TODO: Build index, add embeddings, search by similarity
};

} // namespace librairy

#endif

