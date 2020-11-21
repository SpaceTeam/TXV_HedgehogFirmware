#!/bin/bash

# force enable power
raspi-gpio set 23 op dh
# force enable rpi regulator
raspi-gpio set 24 op dh
# set STM NRST
raspi-gpio set 5 op dh
# set STM NRST enable
raspi-gpio set 26 op dh

sudo openocd -f openocd/openocd_hedgehog_swd.cfg -c "bindto 0.0.0.0"
