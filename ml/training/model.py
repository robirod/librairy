"""
Librairy ML — Model Architectures

Defines audio classification models based on pre-trained audio encoders.
Primary model: Fine-tuned CLAP (Contrastive Language-Audio Pretraining)
for both classification and text-to-audio search.
"""

import torch
import torch.nn as nn
import torchaudio


class MelSpectrogramFrontend(nn.Module):
    """
    Converts raw waveform to log-mel spectrogram.
    Used as the input frontend for CNN-based models.
    """

    def __init__(
        self,
        sample_rate: int = 22050,
        n_mels: int = 128,
        n_fft: int = 2048,
        hop_length: int = 512,
    ):
        super().__init__()
        self.mel_spec = torchaudio.transforms.MelSpectrogram(
            sample_rate=sample_rate,
            n_fft=n_fft,
            hop_length=hop_length,
            n_mels=n_mels,
        )
        self.amplitude_to_db = torchaudio.transforms.AmplitudeToDB(top_db=80)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Args:
            x: (batch, 1, samples) raw waveform
        Returns:
            (batch, 1, n_mels, time) log-mel spectrogram
        """
        spec = self.mel_spec(x)          # (B, 1, n_mels, T)
        spec = self.amplitude_to_db(spec)
        return spec


class AudioClassifier(nn.Module):
    """
    Simple CNN classifier for audio using mel spectrograms.

    Architecture:
        Mel Spectrogram → 4x Conv2D blocks → Global Avg Pool → FC → Softmax

    This serves as a baseline. For production, fine-tune CLAP or PANNs.
    """

    def __init__(
        self,
        num_classes: int,
        sample_rate: int = 22050,
        n_mels: int = 128,
        embedding_dim: int = 512,
    ):
        super().__init__()

        self.frontend = MelSpectrogramFrontend(
            sample_rate=sample_rate, n_mels=n_mels
        )

        # CNN backbone
        self.features = nn.Sequential(
            # Block 1
            nn.Conv2d(1, 64, kernel_size=3, padding=1),
            nn.BatchNorm2d(64),
            nn.ReLU(inplace=True),
            nn.MaxPool2d(2),
            # Block 2
            nn.Conv2d(64, 128, kernel_size=3, padding=1),
            nn.BatchNorm2d(128),
            nn.ReLU(inplace=True),
            nn.MaxPool2d(2),
            # Block 3
            nn.Conv2d(128, 256, kernel_size=3, padding=1),
            nn.BatchNorm2d(256),
            nn.ReLU(inplace=True),
            nn.MaxPool2d(2),
            # Block 4
            nn.Conv2d(256, 512, kernel_size=3, padding=1),
            nn.BatchNorm2d(512),
            nn.ReLU(inplace=True),
            nn.AdaptiveAvgPool2d((1, 1)),
        )

        # Embedding layer
        self.embedding = nn.Sequential(
            nn.Flatten(),
            nn.Linear(512, embedding_dim),
            nn.ReLU(inplace=True),
            nn.Dropout(0.3),
        )

        # Classification head
        self.classifier = nn.Linear(embedding_dim, num_classes)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Args:
            x: (batch, 1, samples) raw audio waveform
        Returns:
            (batch, num_classes) logits
        """
        spec = self.frontend(x)
        features = self.features(spec)
        emb = self.embedding(features)
        logits = self.classifier(emb)
        return logits

    def get_embedding(self, x: torch.Tensor) -> torch.Tensor:
        """
        Extract embedding vector (for similarity search).

        Args:
            x: (batch, 1, samples) raw audio waveform
        Returns:
            (batch, embedding_dim) normalized embedding
        """
        spec = self.frontend(x)
        features = self.features(spec)
        emb = self.embedding(features)
        # L2 normalize for cosine similarity
        emb = nn.functional.normalize(emb, p=2, dim=1)
        return emb

