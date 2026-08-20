"""
registry.py — Signed model registry with firmware-compatibility ranges.

CLAUDE.md §4: "every exported model is signed, versioned, and tagged with
a firmware-compatibility range; the fleet-ota service (§6) must refuse to
distribute a model/firmware combination outside that declared range."

Signing here is HMAC-SHA256 over the artifact's sha256 digest, keyed by a
registry signing key — this simulates ZAKR's *server-side* model-signing
key (a cloud/fleet-ota concern), which is a DIFFERENT key from the
per-device secure element (U14) used for firmware image signing and BLE
attestation (see firmware/secure/). Real production signing would use an
HSM-backed asymmetric key (e.g. Ed25519) rather than a shared HMAC secret
checked into a repo; HMAC is used here only so this module is runnable
and testable without provisioning real key material, and the API shape
(`sign_artifact` / `verify_artifact`) is written so swapping the
implementation later doesn't change any caller.
"""
import hashlib
import hmac
import json
import os
from dataclasses import dataclass, asdict
from typing import Dict, List, Optional, Tuple

REGISTRY_PATH = os.path.join(os.path.dirname(__file__), "registry.json")

# NEVER a real production secret — see module docstring. A real deployment
# pulls this from an HSM/KMS, not a source file.
_DEV_SIGNING_KEY = b"nvband-dev-registry-signing-key-DO-NOT-USE-IN-PRODUCTION"


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def sign_artifact(sha256_hex: str, key: bytes = _DEV_SIGNING_KEY) -> str:
    return hmac.new(key, sha256_hex.encode(), hashlib.sha256).hexdigest()


def verify_artifact_signature(sha256_hex: str, signature: str,
                               key: bytes = _DEV_SIGNING_KEY) -> bool:
    expected = sign_artifact(sha256_hex, key)
    return hmac.compare_digest(expected, signature)


@dataclass
class ModelRegistryEntry:
    model_id: str
    version: str
    artifact_path: str
    sha256: str
    signature: str
    firmware_compat_min: str   # inclusive, semver-like "major.minor.patch"
    firmware_compat_max: str   # inclusive
    data_provenance: str = "synthetic"


def _parse_version(v: str) -> Tuple[int, int, int]:
    parts = v.split(".")
    if len(parts) != 3:
        raise ValueError(f"expected semver-like 'major.minor.patch', got {v!r}")
    return tuple(int(p) for p in parts)  # type: ignore


def register_model(model_id: str, version: str, artifact_path: str,
                    firmware_compat_min: str, firmware_compat_max: str
                    ) -> ModelRegistryEntry:
    if not os.path.exists(artifact_path):
        raise FileNotFoundError(artifact_path)
    if _parse_version(firmware_compat_min) > _parse_version(firmware_compat_max):
        raise ValueError("firmware_compat_min must be <= firmware_compat_max")

    digest = sha256_file(artifact_path)
    signature = sign_artifact(digest)

    entry = ModelRegistryEntry(
        model_id=model_id, version=version, artifact_path=artifact_path,
        sha256=digest, signature=signature,
        firmware_compat_min=firmware_compat_min,
        firmware_compat_max=firmware_compat_max,
    )

    registry = load_registry()
    registry = [e for e in registry
                if not (e.model_id == model_id and e.version == version)]
    registry.append(entry)
    save_registry(registry)
    return entry


def load_registry() -> List[ModelRegistryEntry]:
    if not os.path.exists(REGISTRY_PATH):
        return []
    with open(REGISTRY_PATH) as f:
        raw = json.load(f)
    return [ModelRegistryEntry(**e) for e in raw]


def save_registry(entries: List[ModelRegistryEntry]) -> None:
    with open(REGISTRY_PATH, "w") as f:
        json.dump([asdict(e) for e in entries], f, indent=2)


def verify_entry_integrity(entry: ModelRegistryEntry) -> bool:
    """Re-hashes the artifact on disk and re-verifies the signature —
    this is what fleet-ota calls before EVER distributing an artifact,
    independent of trusting whatever's in registry.json."""
    if not os.path.exists(entry.artifact_path):
        return False
    actual_sha256 = sha256_file(entry.artifact_path)
    if actual_sha256 != entry.sha256:
        return False
    return verify_artifact_signature(entry.sha256, entry.signature)


def is_firmware_compatible(entry: ModelRegistryEntry, firmware_version: str) -> bool:
    fv = _parse_version(firmware_version)
    return (_parse_version(entry.firmware_compat_min) <= fv <=
            _parse_version(entry.firmware_compat_max))


def fleet_ota_would_distribute(entry: ModelRegistryEntry,
                                firmware_version: str) -> Tuple[bool, str]:
    """The single decision point CLAUDE.md §4/§6 describes: refuse to
    distribute a model/firmware combination outside the declared range,
    AND refuse if the artifact fails independent integrity/signature
    re-verification (an entry in registry.json is not trusted just
    because it's there — see verify_entry_integrity)."""
    if not verify_entry_integrity(entry):
        return False, "artifact failed integrity/signature verification"
    if not is_firmware_compatible(entry, firmware_version):
        return False, (f"firmware {firmware_version} outside declared "
                        f"compatibility range [{entry.firmware_compat_min}, "
                        f"{entry.firmware_compat_max}]")
    return True, "ok"


def find_entry(model_id: str, version: str) -> Optional[ModelRegistryEntry]:
    for e in load_registry():
        if e.model_id == model_id and e.version == version:
            return e
    return None


if __name__ == "__main__":
    export_dir = os.path.join(os.path.dirname(__file__), "..", "export")
    artifact = os.path.join(export_dir, "state_classifier_int8.json")
    entry = register_model(
        model_id="nvband_state_classifier",
        version="0.1.0",
        artifact_path=artifact,
        firmware_compat_min="0.1.0",
        firmware_compat_max="0.9.9",
    )
    print(f"registered {entry.model_id} v{entry.version} "
          f"sha256={entry.sha256[:16]}... "
          f"compat=[{entry.firmware_compat_min}, {entry.firmware_compat_max}]")

    ok, reason = fleet_ota_would_distribute(entry, "0.3.0")
    print(f"fleet-ota decision for firmware 0.3.0: {'DISTRIBUTE' if ok else 'REFUSE'} ({reason})")
    ok, reason = fleet_ota_would_distribute(entry, "1.2.0")
    print(f"fleet-ota decision for firmware 1.2.0: {'DISTRIBUTE' if ok else 'REFUSE'} ({reason})")
