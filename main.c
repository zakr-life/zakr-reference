#include "inference_pipeline.h"

int main(void)
{
    pipeline_init();

    while (1)
    {
        pipeline_run();
    }

    return 0;
}
