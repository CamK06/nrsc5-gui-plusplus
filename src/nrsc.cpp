#include <spdlog/spdlog.h>

#include "nrsc.h"

int NRSC5::init(float gain)
{
    if(nrsc5_open(&radio, 0) != 0) { // TODO: Don't just assume device 0 (this will be a GUI option later)
        spdlog::error("Failed to open SDR!");
        return -1;
    }
    if(gain >= 0.0f)
        nrsc5_set_gain(radio, gain);

    return 0;
}

void NRSC5::stop()
{
    nrsc5_stop(radio);
    nrsc5_set_bias_tee(radio, 0);
    nrsc5_close(radio);
}

NRSC5::~NRSC5()
{
    stop();
}