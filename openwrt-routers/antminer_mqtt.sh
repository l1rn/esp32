#!/bin/sh

MINER_IP="192.168.1.100" # miner ip
USER="root"
PASS="root"
MQTT_HOST="127.0.0.1"
TOPIC="antminer/stats"

DATA=$(curl -s --digest -u "$USER:$PASS" \
	"http://$MINER_IP/cgi-bin/stats.cgi")

if [ -n "$DATA" ]; then 
	mosquitto_pub -h "$MQTT_HOST" -t "$TOPIC" -m "$DATA"
fi
