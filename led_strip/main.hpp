#pragma once

#include <string>

enum Pins { VDAC_BASE_PIN       = 0,
            ROW_A_BASE_PIN      = 8, 
            COL_A_BASE_PIN      = 12,
            DAC_EN_PIN          = 16,
            TRIG_PIN            = 17,
            SPI_CLK_PIN         = 18,
            SPI_DOUT_PIN        = 19,
            SPI_COL_EN_PIN      = 20,
            SPI_ALL_EN_PIN      = 21,
            SPARE_PIN           = 22,
            DAC_WR_PIN          = 26,
            DAC_SEL_BASE_PIN    = 27 };
