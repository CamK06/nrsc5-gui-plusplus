#pragma once

/*
    This file and its functions exists to abstract nrsc5 API calls, since they would likely
    be messy to have in the mainwindow.cpp file. This is roughly equivalent to nrsc5.py in
    the Python nrsc5-gui program: https://github.com/cmnybo/nrsc5-gui/blob/master/nrsc5.py
*/

extern "C" {
    #include <nrsc5.h>
}

class NRSC5
{
public:
    int init(float gain);
    void stop();

    ~NRSC5();

private:
    nrsc5_t *radio = NULL;
};