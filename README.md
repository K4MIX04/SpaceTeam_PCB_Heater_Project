# PCB Heating Station with PID Control

## Project overview
Temperature control system for PCB heating using Arduino and PID control.

## Implemented features
- Step response identification
- PID tuning (manual + autotune)
- Data logging to CSV
- Python analysis and plotting

## Repository structure
- src/ – Arduino source code
- data/ – measurement data
- analysis/ – Python scripts
- results/ – generated plots
- docs/ – documentation

## Dependencies

Install via Arduino IDE → Library Manager:

- PID by Brett Beauregard
- MAX6675 library
- OneWire
- SSD1306Ascii by Bill Greiman

Download manually:
- https://github.com/br3ttb/Arduino-PID-AutoTune-Library
