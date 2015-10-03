#!/bin/sh
#
# Example Linux cmd-line script to run the ESP8266 HUZZAH with Stepper Motor
#


# >> CHANGE FOR YOUR IP ADDRESS
ESP_IP="192.168.1.82"

# Show Usage
curl http://$ESP_IP/
sleep 1

# Turn on Motors
curl http://$ESP_IP/stepper/start

# Set RPM
curl http://$ESP_IP/stepper/rpm?10

# Move it!
curl http://$ESP_IP/stepper/steps?50
curl http://$ESP_IP/stepper/steps?-50
sleep 2

# Change speed and Move!
curl http://$ESP_IP/stepper/rpm?50
curl http://$ESP_IP/stepper/steps?200
curl http://$ESP_IP/stepper/steps?-200

# Turn Off Motors
curl http://$ESP_IP/stepper/stop

