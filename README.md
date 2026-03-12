# Encrypted Infrared Data Transmission Proof of Concept
- This repo contains 2 separate main.c files meant to be run on two separate AVR32DA28 microcontrollers. 
- To upload code and manage both MCUs the Platformio extension for VSCode was used.

## Hardware Architecture
The system operates using two unconnected boards communicating via line-of-sight.
* **Microcontrollers**: 2x AVR32DA28 running on internal 4 MHz oscillators.
* **Transmitter Node**: TSAL6400 IR LED (940 nm) connected directly to PD0, driven at approximately 20 mA.
* **Receiver Node**: HS0038B IR demodulator connected to PD3 (configured with internal pull-up). The power supply is stabilized by an RC filter (100 Ω series resistor, 4.7 µF electrolytic capacitor, 100 nF ceramic capacitor).
* **Debug Output**: UART TX mapped to PF0 on both boards.

## Protocol and Modulation
Data is transmitted using a custom implementation of NEC-style pulse distance encoding.
* **Carrier Wave**: 38.46 kHz, generated via software toggling (13 µs HIGH, 13 µs LOW).
* **Encoding Rules**:
  * **Bit '0'**: 21 cycles (~562 µs burst) followed by a 562 µs space.
  * **Bit '1'**: 21 cycles (~562 µs burst) followed by a 1687 µs space.
* **Frame Synchronization**: A 346-cycle (~9 ms) leader burst followed by a 4.5 ms space initiates the transmission. Data bytes are transmitted MSB first.
