"""
grammar.py — the constrained slot-filling generator itself (Addendum 2
§C).

"This is not a free-generation large language model. It is a
constrained, slot-filling generator: every sentence is built from a
small fixed grammar whose slots are bound to named fields of the
structured adaptation/session record. Structurally, the generator cannot
assert a number it wasn't given."

Concretely: every sentence this module produces comes from one of a
small fixed set of Python f-string templates below. Every value that
appears in a template's text is read directly out of the validated input
record's own fields — never computed from an unvalidated default, never
looked up from any other source (no database call, no cache, no prior
sentence). Alongside the text, every sentence carries a `claims` list:
one `FieldClaim(field, asserted_value)` per record field the sentence
states a value for. That list is the ONLY thing `verifier.py` checks —
by design, the verifier never re-parses the English text (see that
module's docstring for why re-parsing would be circular).

`generate()` validates the input record against `record_schema.py`
FIRST. If validation fails, it returns `(None, error_message)` — it
never generates a partial sentence set for an invalid record, and it
never substitutes a default value for a missing field. A generator that
silently used "0" or "unknown" for a missing field would itself be
producing an ungrounded claim (a value not actually traceable to the
record), which is precisely the failure mode this whole module exists to
structurally rule out.
"""
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Tuple

from .record_schema import validate_record


@dataclass(frozen=True)
class FieldClaim:
    """One asserted fact: "this sentence states that `field` has value
    `asserted_value`." `verifier.py` checks each claim independently
    against the source record — it never inspects `text`."""
    field: str
    asserted_value: Any


@dataclass(frozen=True)
class GeneratedSentence:
    text: str
    template_id: str
    claims: Tuple[FieldClaim, ...]


def _direction_word(delta: float) -> str:
    if delta > 0:
        return "increased"
    if delta < 0:
        return "decreased"
    return "did not change"


# ---------------------------------------------------------------------
# adaptation_decision templates
# ---------------------------------------------------------------------

def _generate_adaptation_sentences(data: Dict[str, Any]) -> List[GeneratedSentence]:
    sentences: List[GeneratedSentence] = []

    delta = data["response_delta"]
    sentences.append(GeneratedSentence(
        text=(
            f"For session {data['session_id']}, burst {data['burst_id']}, "
            f"post-stimulation band power {_direction_word(delta)} by "
            f"{abs(delta):.4f} relative to the pre-stimulation baseline "
            f"(response_delta={delta:+.4f})."
        ),
        template_id="adaptation.response_delta.v1",
        claims=(
            FieldClaim("session_id", data["session_id"]),
            FieldClaim("burst_id", data["burst_id"]),
            FieldClaim("response_delta", delta),
        ),
    ))

    current_delta = data["proposed_current_delta_mA"]
    clamped_current = data["clamped_current"]
    clamp_note = " (clamped to the soft training-side bound)" if clamped_current else ""
    sentences.append(GeneratedSentence(
        text=(
            f"The proposed current adjustment for the next burst is "
            f"{current_delta:+.4f} mA{clamp_note}."
        ),
        template_id="adaptation.current_delta.v1",
        claims=(
            FieldClaim("proposed_current_delta_mA", current_delta),
            FieldClaim("clamped_current", clamped_current),
        ),
    ))

    timing_delta = data["proposed_timing_delta_us"]
    clamped_timing = data["clamped_timing"]
    clamp_note = " (clamped to the soft training-side bound)" if clamped_timing else ""
    sentences.append(GeneratedSentence(
        text=(
            f"The proposed timing adjustment for the next burst is "
            f"{timing_delta:+d} microseconds{clamp_note}."
        ),
        template_id="adaptation.timing_delta.v1",
        claims=(
            FieldClaim("proposed_timing_delta_us", timing_delta),
            FieldClaim("clamped_timing", clamped_timing),
        ),
    ))

    sentences.append(GeneratedSentence(
        text=f"This decision was recorded at timestamp {data['timestamp_us']} us.",
        template_id="adaptation.timestamp.v1",
        claims=(FieldClaim("timestamp_us", data["timestamp_us"]),),
    ))

    return sentences


# ---------------------------------------------------------------------
# session_summary templates
# ---------------------------------------------------------------------

def _generate_session_summary_sentences(data: Dict[str, Any]) -> List[GeneratedSentence]:
    sentences: List[GeneratedSentence] = []

    sentences.append(GeneratedSentence(
        text=(
            f"Session {data['session_id']} lasted {data['duration_s']:.1f} seconds "
            f"and included {data['num_stimulation_events']} stimulation event(s)."
        ),
        template_id="session_summary.duration_events.v1",
        claims=(
            FieldClaim("session_id", data["session_id"]),
            FieldClaim("duration_s", data["duration_s"]),
            FieldClaim("num_stimulation_events", data["num_stimulation_events"]),
        ),
    ))

    sentences.append(GeneratedSentence(
        text=(
            f"The average model confidence at stimulation decisions was "
            f"{data['avg_confidence']:.2f}."
        ),
        template_id="session_summary.avg_confidence.v1",
        claims=(FieldClaim("avg_confidence", data["avg_confidence"]),),
    ))

    sentences.append(GeneratedSentence(
        text=(
            f"{data['num_adaptations']} adaptation decision(s) were logged "
            f"during this session."
        ),
        template_id="session_summary.num_adaptations.v1",
        claims=(FieldClaim("num_adaptations", data["num_adaptations"]),),
    ))

    # adherence_pct is optional — only generate a sentence about it when
    # the record actually carries a value. Omitting the sentence when the
    # field is None/absent is the correct behavior; inventing a number
    # ("adherence data unavailable, estimated ~90%") would itself be an
    # ungrounded claim.
    adherence = data.get("adherence_pct")
    if adherence is not None:
        sentences.append(GeneratedSentence(
            text=f"Caregiver-tracked adherence for this session was {adherence:.1f}%.",
            template_id="session_summary.adherence.v1",
            claims=(FieldClaim("adherence_pct", adherence),),
        ))

    return sentences


_GENERATORS = {
    "adaptation_decision": _generate_adaptation_sentences,
    "session_summary": _generate_session_summary_sentences,
}


def generate(record_type: str, data: Dict[str, Any]
             ) -> Tuple[Optional[List[GeneratedSentence]], Optional[str]]:
    """Validates `data` against `record_schema.py` for `record_type`,
    then generates sentences from the fixed template set above.

    Returns `(sentences, None)` on success, or `(None, error_message)`
    if the record fails validation (missing/extra/mistyped field) or
    `record_type` is unknown. Never returns a partial sentence list for
    an invalid record.
    """
    is_valid, errors = validate_record(record_type, data)
    if not is_valid:
        return None, "; ".join(errors)

    generator_fn = _GENERATORS.get(record_type)
    if generator_fn is None:
        return None, f"no generator template set registered for record_type '{record_type}'"

    return generator_fn(data), None
