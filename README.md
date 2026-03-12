# IRE2EMSG
An end-to-end encrypted IR based data transmitter. 
# Encrypted Infrared Data Transmission

[cite_start]This repository contains the bare-metal C implementation of a point-to-point infrared (IR) data transmission system[cite: 5, 17]. [cite_start]It facilitates the transmission of encrypted text strings between two independent AVR32DA28 microcontrollers over a 38 kHz optical link[cite: 5, 6, 16].

## Hardware Architecture
[cite_start]The system operates using two unconnected boards communicating via line-of-sight[cite: 19].
* [cite_start]**Microcontrollers**: 2x AVR32DA28 running on internal 4 MHz oscillators[cite: 25, 52].
* [cite_start]**Transmitter Node**: TSAL6400 IR LED (940 nm) connected directly to PD0, driven at approximately 20 mA[cite: 20, 28, 29, 30].
* [cite_start]**Receiver Node**: HS0038B IR demodulator connected to PD3 (configured with internal pull-up)[cite: 21, 32]. [cite_start]The power supply is stabilized by an RC filter (100 Ω series resistor, 4.7 µF electrolytic capacitor, 100 nF ceramic capacitor)[cite: 43].
* [cite_start]**Debug Output**: UART TX mapped to PF0 on both boards[cite: 62].

## Protocol and Modulation
[cite_start]Data is transmitted using a custom implementation of NEC-style pulse distance encoding[cite: 6, 74].
* [cite_start]**Carrier Wave**: 38.46 kHz, generated via software toggling (13 µs HIGH, 13 µs LOW)[cite: 68, 70].
* **Encoding Rules**:
  * [cite_start]**Bit '0'**: 21 cycles (~562 µs burst) followed by a 562 µs space[cite: 76, 79].
  * [cite_start]**Bit '1'**: 21 cycles (~562 µs burst) followed by a 1687 µs space[cite: 76, 79].
* [cite_start]**Frame Synchronization**: A 346-cycle (~9 ms) leader burst followed by a 4.5 ms space initiates the transmission[cite: 76, 80]. [cite_start]Data bytes are transmitted MSB first[cite: 78].

## Encryption
[cite_start]Payloads are secured using a symmetric XOR cipher[cite: 86]. [cite_start]Each plaintext byte is XOR-encrypted with the static key `0xAA` prior to transmission[cite: 86, 87]. [cite_start]The receiver performs the identical inverse operation upon successful decoding to recover the plaintext[cite: 87].

## Technical Implementation: HS0038B AGC Handling
[cite_start]The HS0038B features an Automatic Gain Control (AGC) circuit designed to suppress continuous signals exceeding 70 cycles (1.8 ms)[cite: 113, 114]. [cite_start]The 9 ms NEC leader burst intentionally violates this constraint, forcing the AGC to classify the signal as noise[cite: 111]. [cite_start]This triggers suppression, causing the output to oscillate and glitch[cite: 114].

[cite_start]The receiver software bypasses this hardware limitation by ignoring the active burst phase[cite: 110, 111]. [cite_start]Synchronization is achieved by exclusively measuring the subsequent stable silence (leader space)[cite: 110]. [cite_start]Subsequent data bursts (21 cycles) remain safely below the AGC suppression threshold[cite: 79].

## Current Limitations
[cite_start]Transmission reliability degrades for payloads exceeding 14 characters[cite: 117]. [cite_start]This error rate correlates directly with accumulated clock drift between the asynchronous 4 MHz internal oscillators on the independent boards[cite: 120]. [cite_start]Future iterations require continuous edge-triggered re-synchronization to support arbitrary-length data frames[cite: 128].
