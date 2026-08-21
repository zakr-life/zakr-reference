"""
record_schema.py — the structured input records the bounded-rationale
generator (Addendum 2 §C) is allowed to generate text from.

"A hallucination, for this codebase, is defined as: a generated sentence
that asserts a specific factual claim ... that is not directly traceable
to a field in the structured record the text was generated from."

That definition only means something if "the structured record" is a
precise, checkable shape — not "whatever dict happened to be passed in."
This module is that shape, for the two record types this pass supports:

1. `AdaptationDecisionRecord` — mirrors, field-for-field, the
   `AdaptationDecision` dataclass already produced by
   `models/training/adaptation.py::decide_adaptation()`, MINUS
   `rationale`. `rationale` there is itself a generated string (built by
   plain f-string formatting in that module); it is deliberately not a
   citable source field here, because a sentence "citing" a previously
   generated string as its justification would be circular — the whole
   point of this module is a second, independent generation path over
   the same *numeric* decision fields, with its own independent verifier
   (`verifier.py`), not a rephrasing of `adaptation.py`'s own rationale
   text.
2. `SessionSummaryRecord` — a new record type (this addendum's own
   design; adherence-adjacent fields intentionally kept in the same
   spirit as the existing caregiver adherence computation in
   `app/mobile/src/state/`) for the clinician-facing session-summary
   feature named in Addendum 2 §C.

`validate_record()` is the single gate `grammar.py` calls before
generating anything. It never fills in a default for a missing required
field — a record that is missing a required field is refused, not
patched, because silently substituting a default is exactly the kind of
value a downstream reader could mistake for an asserted fact.
"""
import typing
from dataclasses import MISSING, dataclass, fields
from typing import Any, Dict, List, Optional, Tuple, Type


@dataclass
class AdaptationDecisionRecord:
    """Field-for-field mirror of
    `models/training/adaptation.py::AdaptationDecision`, minus
    `rationale` (see module docstring). All fields required — every
    field `decide_adaptation()` always populates is always present on a
    real decision record; there is no optional field in this record
    type."""
    session_id: str
    burst_id: int
    timestamp_us: int
    response_delta: float                 # post - pre band power
    proposed_current_delta_mA: float
    proposed_timing_delta_us: int
    clamped_current: bool
    clamped_timing: bool


@dataclass
class SessionSummaryRecord:
    """Clinician-facing session-summary record (Addendum 2 §C, new for
    this pass). `adherence_pct` is optional because caregiver adherence
    tracking is its own opt-in-adjacent feature
    (`app/mobile/src/state/` computes it) — a session summary must still
    be generatable for a session where that figure isn't available,
    without the generator inventing a number to fill the gap."""
    session_id: str
    start_timestamp_us: int
    duration_s: float
    num_stimulation_events: int
    num_adaptations: int                  # count of logged AdaptationDecision records
    avg_confidence: float                 # mean classifier confidence at stim decisions, 0..1
    adherence_pct: Optional[float] = None  # None when adherence tracking is unavailable/off


RECORD_TYPES: Dict[str, Type] = {
    "adaptation_decision": AdaptationDecisionRecord,
    "session_summary": SessionSummaryRecord,
}


def required_field_names(record_cls: Type) -> List[str]:
    return [f.name for f in fields(record_cls)
            if f.default is MISSING and f.default_factory is MISSING]  # type: ignore[misc]


def optional_field_names(record_cls: Type) -> List[str]:
    return [f.name for f in fields(record_cls)
            if not (f.default is MISSING and f.default_factory is MISSING)]  # type: ignore[misc]


def _type_ok(value: Any, expected: Any) -> bool:
    """Minimal structural type check — enough to catch "wrong shape"
    input (a string where a number was declared, etc.) without pulling
    in a validation library. Handles `Optional[X]` (== `Union[X, None]`)
    since several fields use it. `bool` is checked explicitly first
    because Python's `bool` is a subclass of `int` — without this, a
    boolean value would silently satisfy an `int`-typed field."""
    origin = typing.get_origin(expected)
    if origin is typing.Union:
        return any(_type_ok(value, arg) for arg in typing.get_args(expected)
                    if arg is not type(None))
    if expected is bool:
        return isinstance(value, bool)
    if expected is int:
        return isinstance(value, int) and not isinstance(value, bool)
    if expected is float:
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if expected is str:
        return isinstance(value, str)
    return isinstance(value, expected)


def validate_record(record_type: str, data: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """Validates a plain dict against the named record type's schema.

    Returns (is_valid, errors). Deliberately does NOT construct the
    dataclass (which would apply dataclass field defaults) — validation
    must see exactly what the caller supplied, so a required field that
    is absent is reported as missing, never silently treated as "use the
    default" (there are no defaults for required fields here, but the
    principle is the point: this function's job is to say no, not to
    coerce).
    """
    if record_type not in RECORD_TYPES:
        return False, [f"unknown record_type '{record_type}'"]
    if not isinstance(data, dict):
        return False, [f"record data must be a dict, got {type(data).__name__}"]

    record_cls = RECORD_TYPES[record_type]
    hints = typing.get_type_hints(record_cls)
    required = required_field_names(record_cls)
    all_field_names = {f.name for f in fields(record_cls)}

    errors: List[str] = []

    for name in required:
        if name not in data:
            errors.append(f"missing required field '{name}'")
        elif data[name] is None:
            errors.append(f"required field '{name}' is present but None")

    for name in data:
        if name not in all_field_names:
            errors.append(f"unknown field '{name}' is not part of the "
                           f"'{record_type}' schema")

    for name, value in data.items():
        if name not in hints or value is None:
            continue
        if not _type_ok(value, hints[name]):
            errors.append(f"field '{name}' expected type {hints[name]}, "
                           f"got {type(value).__name__} ({value!r})")

    return (len(errors) == 0, errors)
