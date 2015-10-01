#Basement-Monitoring-System

This is a hardware monitor for basement water infiltrations. It is based on a Particle Photon microprocessor and features a DHT22 sensor, a buzzer and two Groove water sensors. The firmware uses the PietteTech_DHT v0.3 library for controlling the DHT22 sensor. Additionally the firmware sends the readings from the DHT22 sensor and water sensors to a Django server database for collection and analisys. To do this I am using Particle created webhooks for the POST (data) requests.

The webhook for sending the data to the Django collection server is:

    {
         "event": "send_owl_data",
         "noDefaults": true,
         "mydevices": true,
         "url":"<my_collection_server>",
         "requestType": "POST",
         "json":{"data":"{{SPARK_EVENT_VALUE}}"}
    }
