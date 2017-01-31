# SmartMeter

This library powers my Wemos D1 mini-based "smart meter".

Its features are quite minimal; it monitors the LED flashes from my electricity meter and periodically
reports the count to an MQTT broker. The reported count is then picked up by a perl script that calculates
the average wattage and ampere per second over the measured period, publishes the results to the
MQTT broker, and logs everything to an SQLite database.

Future expansions will probably include some kind of web chart and possibly Homekit integration.


