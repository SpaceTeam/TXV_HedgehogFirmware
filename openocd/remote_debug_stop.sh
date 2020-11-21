#!/bin/bash

# reset STM NRST enable
raspi-gpio set 26 op dl

# wait for stm to enable power
sleep 2

# reset STM NRST
raspi-gpio set 5 op dh
# disable force enable rpi regulator
raspi-gpio set 24 op dl
# disable force enable power
raspi-gpio set 23 op dl

