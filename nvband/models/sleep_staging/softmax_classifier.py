"""
softmax_classifier.py — Minimal multinomial logistic regression, pure
Python standard library (no numpy/torch/tensorflow), for 5-class sleep
staging (Wake/N1/N2/N3/REM).

Deliberately identical in structure to
`models/training/softmax_classifier.py` (duplicated rather than imported
so this package stays self-contained) — a small, auditable model
appropriate for a Cortex-M33 inference budget, not a claim that a linear
classifier is clinically sufficient. The training/export machinery around
it (subject-level splits, artifact-robustness eval, quantization) is the
part that matters; swapping in a larger architecture later does not
change any of that.
"""
import json
import math
import random
from typing import Dict, List


class SoftmaxClassifier:
    def __init__(self, num_features: int, num_classes: int):
        self.num_features = num_features
        self.num_classes = num_classes
        self.weights = [[0.0] * num_features for _ in range(num_classes)]
        self.bias = [0.0] * num_classes
        self.feature_mean = [0.0] * num_features
        self.feature_std = [1.0] * num_features

    def fit_normalization(self, X: List[List[float]]) -> None:
        n = len(X)
        for j in range(self.num_features):
            col = [row[j] for row in X]
            mean = sum(col) / n
            var = sum((v - mean) ** 2 for v in col) / n
            self.feature_mean[j] = mean
            self.feature_std[j] = math.sqrt(var) if var > 1e-9 else 1.0

    def _normalize(self, x: List[float]) -> List[float]:
        return [(x[j] - self.feature_mean[j]) / self.feature_std[j]
                for j in range(self.num_features)]

    def _logits(self, x_norm: List[float]) -> List[float]:
        out = []
        for c in range(self.num_classes):
            s = self.bias[c]
            w = self.weights[c]
            for j in range(self.num_features):
                s += w[j] * x_norm[j]
            out.append(s)
        return out

    @staticmethod
    def _softmax(logits: List[float]) -> List[float]:
        m = max(logits)
        exps = [math.exp(v - m) for v in logits]
        total = sum(exps)
        return [v / total for v in exps]

    def predict_proba(self, x: List[float]) -> List[float]:
        return self._softmax(self._logits(self._normalize(x)))

    def predict(self, x: List[float]) -> int:
        probs = self.predict_proba(x)
        return max(range(len(probs)), key=lambda i: probs[i])

    def train(self, X: List[List[float]], y: List[int],
              learning_rate: float = 0.2, l2: float = 1e-3,
              epochs: int = 60, seed: int = 1) -> List[float]:
        """Full-batch gradient descent. Returns per-epoch training loss
        (cross-entropy + L2) for logging/plots."""
        self.fit_normalization(X)
        Xn = [self._normalize(x) for x in X]
        n = len(Xn)
        rng = random.Random(seed)
        order = list(range(n))
        loss_history = []

        for epoch in range(epochs):
            rng.shuffle(order)
            grad_w = [[0.0] * self.num_features for _ in range(self.num_classes)]
            grad_b = [0.0] * self.num_classes
            total_loss = 0.0

            for idx in order:
                x = Xn[idx]
                target = y[idx]
                probs = self._softmax(self._logits(x))
                total_loss += -math.log(max(probs[target], 1e-12))

                for c in range(self.num_classes):
                    err = probs[c] - (1.0 if c == target else 0.0)
                    grad_b[c] += err
                    wc = grad_w[c]
                    for j in range(self.num_features):
                        wc[j] += err * x[j]

            for c in range(self.num_classes):
                for j in range(self.num_features):
                    grad = grad_w[c][j] / n + l2 * self.weights[c][j]
                    self.weights[c][j] -= learning_rate * grad
                self.bias[c] -= learning_rate * (grad_b[c] / n)

            loss_history.append(total_loss / n)

        return loss_history

    def to_dict(self) -> Dict:
        return {
            "num_features": self.num_features,
            "num_classes": self.num_classes,
            "weights": self.weights,
            "bias": self.bias,
            "feature_mean": self.feature_mean,
            "feature_std": self.feature_std,
        }

    @classmethod
    def from_dict(cls, d: Dict) -> "SoftmaxClassifier":
        m = cls(d["num_features"], d["num_classes"])
        m.weights = d["weights"]
        m.bias = d["bias"]
        m.feature_mean = d["feature_mean"]
        m.feature_std = d["feature_std"]
        return m

    def save(self, path: str) -> None:
        with open(path, "w") as f:
            json.dump(self.to_dict(), f, indent=2)

    @classmethod
    def load(cls, path: str) -> "SoftmaxClassifier":
        with open(path) as f:
            return cls.from_dict(json.load(f))
