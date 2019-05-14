# MQTT
libmosquitto is a C based library for MQTT protocol implementation and usage.
This is a simple MQTT client over libmosquitto, with the facility to optionally daemonize
the client subscriber.

Example command :

./client -s 172.16.1.105 -p 1887		# DEFAULT PORT IS 1883

Done:
1. Added support for publishing data after subscribing, publishing on RESULT.
2. Need to add support for port and ip address under getopt.

To Do:
1. Collect data from other applications.
