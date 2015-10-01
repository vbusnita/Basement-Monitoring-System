#Basement-Monitoring-System

This is a hardware monitor for basement water infiltrations. It is based on a Particle Photon microprocessor and features a DHT22 sensor, a buzzer and two Groove water sensors. The firmware uses the PietteTech_DHT v0.3 library for controlling the DHT22 sensor. Additionally the firmware sends the readings from the DHT22 sensor and water sensors to a Django server database for collection and analisys. To do this I am using Particle created webhooks for the POST (data) requests.

**The webhook for sending the data to the Django collection server is:**

    {
         "event": "send_owl_data",
         "noDefaults": true,
         "mydevices": true,
         "url":"<my_collection_server>",
         "requestType": "POST",
         "json":{"data":"{{SPARK_EVENT_VALUE}}"}
    }

**The webhook for the hue_alarm_start request is:**

    {
        "event": "hue_alarm_start",
        "noDefaults": true,
        "mydevices": true,
        "url": "<my_hue_server>",
        "requestType": "PUT",
        "json": {
          "on":true,
          "bri":254,
          "sat":255,
          "hue":65535,
          "alert":"lselect"
        }
    }

**The webhook for Pushover.net request is:**

    {
        "eventName": "basement_leak",
        "url": "https://api.pushover.net/1/messages.json",
        "requestType": "POST",
        "query":
          {
          "user": "<my_user>",
          "token": "<my_token>",
          "title": "Basement Monitor",
          "message": "{{SPARK_EVENT_VALUE}}"
          },
        "mydevices": true
    }
