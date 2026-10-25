"""
Librairy ML — Audio Dataset Loader

Loads audio files organized in folder-per-class structure:
    dataset/
    ├── Ambience/
    │   ├── city_01.wav
    │   └── forest_02.wav
    ├── SFX/
    │   └── explosion_01.wav
    ...

Supports on-the-fly augmentation and mel spectrogram extraction.
"""

import os
from pathlib import Path
from typing import Optional

import torch
import torchaudio
from torch.utils.data import Dataset


class AudioDataset(Dataset):
    """
    PyTorch dataset for audio classification.

    Args:
        root_dir:      Root directory containing class subfolders
        sample_rate:   Target sample rate (resamples if needed)
        duration:      Target duration in seconds (crops or pads)
        augment:       Enable data augmentation
        extensions:    Supported file extensions
    """

    SUPPORTED_EXTENSIONS = {".wav", ".flac", ".mp3", ".ogg", ".aiff", ".aif"}

    def __init__(
        self,
        root_dir: str,
        sample_rate: int = 22050,
        duration: float = 5.0,
        augment: bool = False,
        extensions: Optional[set] = None,
    ):
        self.root_dir = Path(root_dir)
        self.sample_rate = sample_rate
        self.duration = duration
        self.target_samples = int(sample_rate * duration)
        self.augment = augment
        self.extensions = extensions or self.SUPPORTED_EXTENSIONS

        # Discover classes and files
        self.classes: list[str] = sorted(
            d.name
            for d in self.root_dir.iterdir()
            if d.is_dir() and not d.name.startswith(".")
        )
        self.class_to_idx: dict[str, int] = {
            cls: idx for idx, cls in enumerate(self.classes)
        }

        self.samples: list[tuple[Path, int]] = []
        for cls in self.classes:
            cls_dir = self.root_dir / cls
            for f in sorted(cls_dir.rglob("*")):
                if f.suffix.lower() in self.extensions:
                    self.samples.append((f, self.class_to_idx[cls]))

    def __len__(self) -> int:
        return len(self.samples)

    def __getitem__(self, idx: int) -> tuple[torch.Tensor, int]:
        path, label = self.samples[idx]

        # Load audio
        waveform, sr = torchaudio.load(str(path))

        # Resample if needed
        if sr != self.sample_rate:
            resampler = torchaudio.transforms.Resample(sr, self.sample_rate)
            waveform = resampler(waveform)

        # Convert to mono
        if waveform.shape[0] > 1:
            waveform = waveform.mean(dim=0, keepdim=True)

        # Pad or crop to target duration
        if waveform.shape[1] < self.target_samples:
            padding = self.target_samples - waveform.shape[1]
            waveform = torch.nn.functional.pad(waveform, (0, padding))
        elif waveform.shape[1] > self.target_samples:
            if self.augment:
                # Random crop during training
                max_start = waveform.shape[1] - self.target_samples
                start = torch.randint(0, max_start, (1,)).item()
            else:
                start = 0
            waveform = waveform[:, start : start + self.target_samples]

        # Augmentation
        if self.augment:
            waveform = self._augment(waveform)

        return waveform, label

    def _augment(self, waveform: torch.Tensor) -> torch.Tensor:
        """Apply random augmentations."""
        # Random gain (-6dB to +6dB)
        if torch.rand(1).item() < 0.5:
            gain_db = (torch.rand(1).item() - 0.5) * 12
            gain = 10 ** (gain_db / 20)
            waveform = waveform * gain

        # Add Gaussian noise
        if torch.rand(1).item() < 0.3:
            noise = torch.randn_like(waveform) * 0.005
            waveform = waveform + noise

        return waveform

    @property
    def num_classes(self) -> int:
        return len(self.classes)

