#!/bin/sh
#
# Example client for
# sketch/StepperServerHardened/StepperServerHardened.ino
#

set -eu

ESP_HOST="${1:-192.168.1.82}"
: "${STEPPER_API_KEY:?Set STEPPER_API_KEY to the value in StepperConfig.h}"

motor_started=0

request() {
  curl \
    --fail \
    --show-error \
    --silent \
    --max-time 5 \
    --request POST \
    --header "X-API-Key: ${STEPPER_API_KEY}" \
    "http://${ESP_HOST}$1"
}

cleanup() {
  if [ "$motor_started" -eq 1 ]; then
    request "/stepper/stop" >/dev/null 2>&1 || true
  fi
}

trap cleanup EXIT HUP INT TERM

curl \
  --fail \
  --show-error \
  --silent \
  --max-time 5 \
  "http://${ESP_HOST}/"
printf '\n'

request "/stepper/start"
printf '\n'
motor_started=1

request "/stepper/rpm?value=10"
printf '\n'
request "/stepper/steps?value=50"
printf '\n'
request "/stepper/steps?value=-50"
printf '\n'

request "/stepper/rpm?value=50"
printf '\n'
request "/stepper/steps?value=200"
printf '\n'
request "/stepper/steps?value=-200"
printf '\n'

request "/stepper/stop"
printf '\n'
motor_started=0
