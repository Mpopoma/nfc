![Happy Bubbles Logo](happy_bubbles_logo.png?raw=true)

# Happy Bubbles - Wifi NFC Device

This device is in the "Happy Bubbles" series of open hardware and open source IoT and home-automation devices. The Wifi NFC device is easy to use, easy to make yourself, and cheap. 

Its purpose is to send NFC/RFID tags that you touch to it to an HTTP URL on whatever server you'd like.

What you need to build it:
 * a NodeMCU ESP8266 development kit - [buy one here](http://amzn.to/1S03qNE)
 * an RC522 RFID module - [buy one here](http://amzn.to/1U1rVxE)
 * (optional) a WS2812B RGB LED

## How to set it up

#### 0. Initial flashing

Easiest is to download the  release and use your favorite ESP8266 flashing tool to flash the bootloader, the firmware, and blank settings. Detailed instructions are provided in the release notes, thanks tve!

#### 1. Wire it up 
| RC522 NFC pin   |      NodeMCU pin      |  ESP8266 GPIO |
|----------|:-------------:|------:|
| SDA | D8 | GPIO 15 |
| SCK | D5 | GPIO 14 |
| MOSI | D7 | GPIO 13 |
| MISO | D6 | GPIO 12 |
| IRQ | Not connected| Not connected |
| GND | Ground / GND | Ground |
| RST | D4 | GPIO 2 |
| 3.3V | 3.3V / VCC | VCC |

If there is interest, I can create a PCB for you guys to make this easier. Let me know if anyone would like these and I could put them up for sale.

#### 2. Optional WS2812B LED

An RGB LED is useful as a status indicator to tell you when you're in config-mode, when a tag is hit, and the server response can even change its colour!

| WS2812 pin   |      NodeMCU pin      |  ESP8266 GPIO |
|----------|:-------------:|------:|
| GND | GND | GND |
| Data-In | D1 | GPIO 5 |
| 5V Power | VIN | not applicable |

#### 3. Powering it up
Just insert a microUSB cable in the NodeMCU! When it boots up, its red LED should be lit. If you have a WS2812b hooked up, it will glow orange.

If it doesn't, try holding the "FLASH: button on the nodeMCU board held down for about 10 seconds. Holding the button down lets to change between config mode and RFID-scanning mode. I left these two seperate for security, so that while your device is acting as an RFID scanner, someone can't just connect to its hostname and start messing with your settings.

#### 4. Connect it to your wifi network
1. With the board powered up, the Happy Bubbles NFC device will automatically start an access point you can connect to called "happy-bubbles-nfc". 
2. Connect to this access point with your computer
3. Open a browser and visit http://192.168.4.1/
4. You'll now be at the Happy Bubbles config screen! You can change the hostname of the device here.
5. On the left side with the navigation bar, go to "WiFi Setup". It will populate a bunch of wifi access points it can see. Find yours and type in the password. You should see it get an IP address from it, and now you're connected!

#### 5. Make it hit your server
1. From the same  config mode, go to "NFC Setup". In there, you can set some parameters for how the JSON request from the Happy Bubbles device will go to your server.
2. The most important one here is the HTTP URL, this URL will receive a POST request from the device when a tag is scanned.
3. The 'Device ID' is a name you can give this device that will also be included in the JSON. This is useful if you have more than one Happy Bubbles NFC device around and you want to know which one the request is coming from.
4. The 'Device Secret' and 'Device counter' are optional but useful if you want to authenticate the requests to make sure someone isn't spoofing a tag. 

#### 5. Make it run!

Hold down the "FLASH" button on the NodeMCU board for about 10 seconds. The red LED on the NodeMCU should turn on, and if you have an RGB LED hooked up, it will blink purple a few times, fast. It's now reading tags! Every time you touch a tag, the RGB LED will light pink quickly.

## Communication

#### HTTP Request
Every time you touch a tag to the device, it will send an HTTP POST request to the URL you specified in the config. The REQUEST payload will be JSON that looks like this:
```
{
  "device_id": "CONFIG-DEVICE-ID", 
  "tag_uid": "THE-TAGS-UUID", 
  "counter": 3, 
  "hash": "7cac4442e6caf834ce20a359ff7ee9ff34525691"
}
```

The "device_id" is the "Device ID" you specified in the config. The "tag_uuid" is the unique ID of the tag that was detected by the reader. The "counter" and "hash" are optional security authentication parameters described below in the "Optional Authentication" section.

#### HTTP Response
After you touch the tag, the device expects a 200 OK response with a JSON payload. Don't worry about having to send one, it's optional:
```
{
  "r":5,
  "g":250,
  "b":2,
  "delay":2000
}
```

The "r", "g", and "b" are RGB colour values for the optional WS2812B LED. The server could tell the LED to light up a certain way after the request. This way you could give the user some kind of indication of what happened. 

The "delay" is the number of milliseconds to keep the color after a request. If you make it 0, the LED will stay lit that way forever, or until the next reqest...or until you remove power!



## Optional Authentication
The way this authentication works is that the Happy Bubbles NFC device will send a "hash" string in its JSON response. This "hash" is an SHA1-based HMAC, its input is the device-id, the NFC tag-ID, and the counter all concatenated together with no spaces. The 'counter' increments every time your server sends back a 200 OK response. It is used as a 'nonce' to prevent replay attacks on this authentication mechanism.
This string is then SHA1-HMAC'd hashed against the "device secret" from the config menu. You can use this on the backend to ensure no one is spoofing tag-hits, it's optional.


### Credits

This project wouldn't have been possible without my standing on the shoulders of giants such as:
 * [NodeMCU](https://github.com/nodemcu/nodemcu-devkit-v1.0) - The ESP8266-based hardware that this project uses.
 * [esp-link](https://github.com/jeelabs/esp-link) - The firmware for this project is a modified subset of the what JeeLabs' "esp-link" project has built and made this whole thing really nice and easy to work with. 
 * [Espressif](http://espressif.com) - for making the amazing ESP8266 project that enables things like this to even be possible. A wonderful product from a wonderful company.
 * [esphttpd](http://www.esp8266.com/wiki/doku.php?id=esp-httpd) - SpriteTM made an awesome little HTTP webserver for the ESP8266 that powers the config menu of the esp-link and the Happy Bubbles NFC device
 * everyone in the ESP8266 community who shares their code and advice.

