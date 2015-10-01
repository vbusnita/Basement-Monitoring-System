#Basement-Monitoring-System

This is a hardware monitor for basement water infiltrations. It is based on a Particle Photon microprocessor and features a DHT22 sensor, a buzzer and two Groove water sensors. The firmware uses the PietteTech_DHT v0.3 library for controlling the DHT22 sensor. Additionally the firmware sends the readings from the DHT22 sensor and water sensors to a Django server database for collection and analisys. To do this I am using Particle created webhooks for the POST requests. More information on this in the works.
