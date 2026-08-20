#include "brainprint_auth_gate.h"

nvband_brainprint_gate_result_t nvband_brainprint_gate_evaluate(bool brainprint_matched)
{
    return brainprint_matched
        ? NVBAND_BRAINPRINT_GATE_RESULT_MATCH
        : NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK;
}
