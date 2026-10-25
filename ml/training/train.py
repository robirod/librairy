"""
Librairy ML — Training Script

Train audio classification model and export to ONNX.

Usage:
    python -m training.train \\
        --data_dir /path/to/dataset \\
        --output_dir ./checkpoints \\
        --epochs 50 \\
        --batch_size 32
"""

import argparse
from pathlib import Path

import torch
import torch.nn as nn
from torch.utils.data import DataLoader, random_split
from tqdm import tqdm

from .dataset import AudioDataset
from .model import AudioClassifier


def train(args: argparse.Namespace) -> None:
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Training on: {device}")

    # Dataset
    full_dataset = AudioDataset(
        root_dir=args.data_dir,
        sample_rate=args.sample_rate,
        duration=args.duration,
        augment=True,
    )

    print(f"Dataset: {len(full_dataset)} samples, "
          f"{full_dataset.num_classes} classes")
    print(f"Classes: {full_dataset.classes}")

    # Train/val split (80/20)
    train_size = int(0.8 * len(full_dataset))
    val_size = len(full_dataset) - train_size
    train_dataset, val_dataset = random_split(
        full_dataset, [train_size, val_size]
    )

    train_loader = DataLoader(
        train_dataset,
        batch_size=args.batch_size,
        shuffle=True,
        num_workers=args.num_workers,
        pin_memory=True,
    )
    val_loader = DataLoader(
        val_dataset,
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=args.num_workers,
        pin_memory=True,
    )

    # Model
    model = AudioClassifier(
        num_classes=full_dataset.num_classes,
        sample_rate=args.sample_rate,
    ).to(device)

    # Loss & optimizer
    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.AdamW(
        model.parameters(), lr=args.lr, weight_decay=1e-4
    )
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
        optimizer, T_max=args.epochs
    )

    # Output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    best_val_acc = 0.0

    for epoch in range(args.epochs):
        # ---- Train ----
        model.train()
        train_loss = 0.0
        train_correct = 0
        train_total = 0

        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{args.epochs}")
        for waveforms, labels in pbar:
            waveforms = waveforms.to(device)
            labels = labels.to(device)

            optimizer.zero_grad()
            logits = model(waveforms)
            loss = criterion(logits, labels)
            loss.backward()
            optimizer.step()

            train_loss += loss.item()
            preds = logits.argmax(dim=1)
            train_correct += (preds == labels).sum().item()
            train_total += labels.size(0)

            pbar.set_postfix(
                loss=f"{loss.item():.4f}",
                acc=f"{train_correct/train_total:.3f}"
            )

        scheduler.step()

        # ---- Validate ----
        model.eval()
        val_correct = 0
        val_total = 0

        with torch.no_grad():
            for waveforms, labels in val_loader:
                waveforms = waveforms.to(device)
                labels = labels.to(device)

                logits = model(waveforms)
                preds = logits.argmax(dim=1)
                val_correct += (preds == labels).sum().item()
                val_total += labels.size(0)

        val_acc = val_correct / val_total if val_total > 0 else 0.0

        print(f"  Val Accuracy: {val_acc:.4f} "
              f"({'BEST' if val_acc > best_val_acc else ''})")

        # Save best model
        if val_acc > best_val_acc:
            best_val_acc = val_acc
            torch.save(
                {
                    "epoch": epoch,
                    "model_state_dict": model.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                    "val_acc": val_acc,
                    "classes": full_dataset.classes,
                    "num_classes": full_dataset.num_classes,
                    "sample_rate": args.sample_rate,
                    "duration": args.duration,
                },
                output_dir / "best_model.pt",
            )

    print(f"\nTraining complete. Best val accuracy: {best_val_acc:.4f}")
    print(f"Model saved to: {output_dir / 'best_model.pt'}")


def main():
    parser = argparse.ArgumentParser(
        description="Train Librairy audio classifier"
    )
    parser.add_argument(
        "--data_dir", type=str, required=True,
        help="Root directory with class subfolders"
    )
    parser.add_argument(
        "--output_dir", type=str, default="./checkpoints",
        help="Directory to save checkpoints"
    )
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch_size", type=int, default=32)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--sample_rate", type=int, default=22050)
    parser.add_argument("--duration", type=float, default=5.0)
    parser.add_argument("--num_workers", type=int, default=4)
    args = parser.parse_args()

    train(args)


if __name__ == "__main__":
    main()

