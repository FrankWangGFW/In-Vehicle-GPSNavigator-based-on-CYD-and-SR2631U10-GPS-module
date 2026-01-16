# GPS Navigation System - Based on Arduino and TFT Display
Source code for a car GPS navigation project using ESP32-2432S028 (Cheap Yellow Display) and SR2631U10 GPS module
A GPS navigation system built with Arduino IDE, featuring real-time position tracking, direction display, speed monitoring, and satellite information on a TFT screen.

Finished.jpg

## Project Overview

This project implements a GPS navigation display system using ESP32-2432S028 (CYD) and SR2631U10 GPS module. The system receives GPS data, processes it, and displays essential navigation information including:
- Latitude and longitude coordinates
- Altitude
- Current speed
- Compass direction (with freeze feature at low speeds)
- Current time and date (converted to Beijing time)
- Satellite count and signal accuracy

## Hardware Requirements

- ESP32-2432S028 development board with 2.8-inch display (320x240 resolution), based on ESP32-D0WDQ6 controller (known as Cheap Yellow Display, CYD)
- GPS module (supporting NMEA protocol), I used SR2631U10 which is widely used in FPV drones
- UART to 4-pin DuPont cable
- Type-C or Mini-USB power supply

## Software Requirements

- Arduino IDE
- Required libraries:
  - TFT_eSPI.h (for TFT display)
  - HardwareSerial.h (for serial communication)
  - TinyGPSPlus.h (for GPS data parsing)
  - math.h (for mathematical calculations)

## Installation Instructions

1. Clone or download this repository to your local machine
2. Open the Arduino IDE
3. Install the required libraries via Library Manager
4. Open the `GPSnavi_based_on_CYD_and_SR2631U10.ino` file
5. Configure the TFT display settings in User_Setup.h if needed
6. Upload the code to the ESP32-2432S028 development board

## GPS and TFT Display UART Interface Pin Configuration

| Component | Pin |
|-----------|-----|
| GPS RX    | 22  |
| GPS TX    | 27  |
| VCC       | 3.3V|
| GND       | GND |

## Usage Instructions

1. Connect all hardware components according to the pin configuration
2. Power on the system
3. Wait for the GPS module to acquire satellite signals (searching screen will be displayed)
4. Once satellite signals are acquired, the main navigation screen will be displayed
5. The system will automatically update with real-time GPS data

## Features

### Display Panels

#### Left Panel
- Latitude and longitude coordinates
- Altitude (in meters)
- Current speed (in km/h)

#### Right Panel
- Compass direction with angle difference
- Current time (Beijing time)
- Current date

#### Header Area
- Satellite count
- Position accuracy (in meters)
- Satellite icon indicator

### Direction System
- Real-time direction calculation based on heading
- Direction freeze feature at low speeds (< 4 km/h)
- Displays main direction (NORTH, EAST, SOUTH, WEST)
- Displays secondary direction when angle difference > 10°

### Time and Date
- Automatic conversion from UTC to Beijing time
- Proper date handling across time zone boundaries
- Efficient time display updates (only changes when necessary)

### GPS Features
- Real-time position tracking
- Speed monitoring
- Altitude display
- Satellite count and accuracy information
- Signal searching screen with status updates

## Code Structure

### Main Functions

- `setup()`: Initializes hardware and display
- `loop()`: Main program loop, handles GPS data parsing and display updates
- `parseGPSData()`: Processes raw GPS data from the module
- `updateLeftPanel()`: Updates position and speed information
- `updateRightPanel()`: Updates direction, time and date information
- `getDirection()`: Calculates compass direction from heading angle
- `utcToBeijingTime()`: Converts UTC time to Beijing time
- `utcToBeijingDate()`: Converts UTC date to Beijing date
- `showSearchingScreen()`: Displays GPS signal searching screen
- `switchToMainUI()`: Switches from searching screen to main navigation screen

### Configuration Constants

The code includes various configurable constants:

- GPS baud rate and pin assignments
- Update intervals for display components
- Color definitions for UI elements
- Panel section dimensions

## About nissan_logo.h and satellite_icon_bw.h Header Files

nissan_logo.h: Since my car is a Nissan X-Trail, I created a bitmap containing the Nissan logo for display on the searching screen. You can generate your preferred icon using a bitmap generator.

satellite_icon_bw.h: Contains bitmap data for a black and white satellite icon, used on the main navigation screen.

## Customization Tips

1. **Time Zone**: Modify the `utcToBeijingTime()` and `utcToBeijingDate()` functions to use your local time zone instead of Beijing time
2. **Display Colors**: Update the color definitions in the "Color Definitions" section to change the UI appearance
3. **Update Intervals**: Adjust `UPDATE_INTERVAL` and `SEARCH_UPDATE_INTERVAL` to change how frequently the display updates
4. **Screen Rotation**: Change `TFT_ROTATION` to adjust the screen orientation
5. **Direction Freeze Speed**: Modify the speed threshold in `updateRightPanel()` where direction freezing occurs

## Troubleshooting

### GPS Signal Issues
- Ensure the GPS module has a clear view of the sky
- Check the GPS module wiring and baud rate setting
- Wait several minutes for the GPS module to acquire satellite signals initially

### Display Issues
- Verify TFT display connections
- Check User_Setup.h configuration for your specific TFT display model
- Ensure the correct rotation is set with `TFT_ROTATION`

### Serial Communication Issues
- Verify GPS RX/TX pin assignments
- Check if the GPS baud rate matches the module's configuration

## License

This project is open source and available under the MIT License.

## Acknowledgments

- Uses the TinyGPSPlus library for GPS data parsing
- Uses the TFT_eSPI library for TFT display control

- Thanks to China's advanced manufacturing industry for providing low-cost, high-quality components


