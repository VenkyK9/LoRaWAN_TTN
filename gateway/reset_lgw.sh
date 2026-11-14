#!/bin/bash
#
# Placeholder reset script.
#
# lora_pkt_fwd always runs "./reset_lgw.sh start" from its own folder and
# aborts with "failed to reset SX1302" if the script is missing or fails.
# The real reset is done by reset_gpio.py before the forwarder starts, so
# this file only has to exist and exit cleanly.

echo "reset_lgw.sh placeholder: reset already done by reset_gpio.py"
exit 0
