#!/bin/bash
#
# Reset the concentrator, then start the packet forwarder.
# Copy the three files in this folder into ~/sx1302_hal/packet_forwarder/
# and run:  ./start_gateway.sh
#
# Stop with Ctrl+C. "PULL_ACK received" lines in the output mean the
# gateway is connected to TTN.

cd ~/sx1302_hal/packet_forwarder || exit 1

sudo python3 reset_gpio.py
sudo ./lora_pkt_fwd
