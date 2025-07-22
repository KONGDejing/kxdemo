
#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"


class LedBlink {
public:
    static void init();

    static void start();
    static void stop();

    static void red_slow_flashing();
    static void green_slow_flashing();
    static void blue_slow_flashing();

    static void red_all();
    static void green_all();
    static void blue_all();

    static void red_fast_flashing();
    static void green_fast_flashing();
    static void blue_fast_flashing();

    static void green_marquee();

private:
    

};

