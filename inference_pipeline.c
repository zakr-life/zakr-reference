#include "inference_pipeline.h"
#include "sensor_fusion.h"
#include "feature_extract.h"

static float feature_vector[60];

void pipeline_init(void)
{
    sensor_fusion_init();
}

void pipeline_run(void)
{
    sensor_frame_t frame;

    sensor_fusion_update(&frame);
    extract_features(&frame, feature_vector);
}
