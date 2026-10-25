"""
Librairy ML — ONNX Export Script

Exports a trained PyTorch model to ONNX format for inference in the C++ app.

Usage:
    python -m training.export_onnx \\
        --checkpoint ./checkpoints/best_model.pt \\
        --output ./models/librairy_classifier.onnx
"""

import argparse
from pathlib import Path

import torch
import onnx
import onnxruntime as ort
import numpy as np

from .model import AudioClassifier


def export_to_onnx(args: argparse.Namespace) -> None:
    # Load checkpoint
    checkpoint = torch.load(args.checkpoint, map_location="cpu",
                            weights_only=False)

    num_classes = checkpoint["num_classes"]
    sample_rate = checkpoint["sample_rate"]
    duration = checkpoint["duration"]
    classes = checkpoint["classes"]

    print(f"Model: {num_classes} classes, SR={sample_rate}, "
          f"duration={duration}s")
    print(f"Classes: {classes}")

    # Recreate model and load weights
    model = AudioClassifier(
        num_classes=num_classes,
        sample_rate=sample_rate,
    )
    model.load_state_dict(checkpoint["model_state_dict"])
    model.eval()

    # Dummy input: (batch=1, channels=1, samples)
    num_samples = int(sample_rate * duration)
    dummy_input = torch.randn(1, 1, num_samples)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Export
    print(f"Exporting to: {output_path}")
    torch.onnx.export(
        model,
        dummy_input,
        str(output_path),
        input_names=["waveform"],
        output_names=["logits"],
        dynamic_axes={
            "waveform": {0: "batch_size"},
            "logits": {0: "batch_size"},
        },
        opset_version=17,
    )

    # Validate ONNX model
    onnx_model = onnx.load(str(output_path))
    onnx.checker.check_model(onnx_model)
    print("✅ ONNX model validation passed")

    # Verify output matches PyTorch
    with torch.no_grad():
        pytorch_output = model(dummy_input).numpy()

    session = ort.InferenceSession(str(output_path))
    onnx_output = session.run(
        None, {"waveform": dummy_input.numpy()}
    )[0]

    max_diff = np.max(np.abs(pytorch_output - onnx_output))
    print(f"Max output difference (PyTorch vs ONNX): {max_diff:.8f}")
    assert max_diff < 1e-4, f"Output mismatch! max_diff={max_diff}"
    print("✅ Output verification passed")

    # Also export embedding model variant
    embedding_path = output_path.with_stem(output_path.stem + "_embedding")
    print(f"Exporting embedding model to: {embedding_path}")

    class EmbeddingWrapper(torch.nn.Module):
        def __init__(self, model):
            super().__init__()
            self.model = model

        def forward(self, x):
            return self.model.get_embedding(x)

    emb_model = EmbeddingWrapper(model)
    emb_model.eval()

    torch.onnx.export(
        emb_model,
        dummy_input,
        str(embedding_path),
        input_names=["waveform"],
        output_names=["embedding"],
        dynamic_axes={
            "waveform": {0: "batch_size"},
            "embedding": {0: "batch_size"},
        },
        opset_version=17,
    )
    print("✅ Embedding model exported")

    # Save class list alongside the model
    classes_path = output_path.with_suffix(".classes.txt")
    with open(classes_path, "w") as f:
        for cls in classes:
            f.write(cls + "\n")
    print(f"✅ Class list saved to: {classes_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Export trained model to ONNX"
    )
    parser.add_argument(
        "--checkpoint", type=str, required=True,
        help="Path to PyTorch checkpoint (.pt)"
    )
    parser.add_argument(
        "--output", type=str, default="./models/librairy_classifier.onnx",
        help="Output ONNX file path"
    )
    args = parser.parse_args()

    export_to_onnx(args)


if __name__ == "__main__":
    main()

