CHANGELOG
DomeLift v1.1-fbn

Base: Printed-Droid / Matthew Zwarts (v1.x)
Author of modifications: NandoMadrid

Added
Periscope light control

Added digital output on pin 40 (PERISC_LIGHT_PIN) to control periscope lighting via external transistor.

Light turns ON while the periscope is not in the bottom position.

Light turns OFF immediately when the bottom limit switch (PBot) is reached.

Logic is based on the physical limit switch state, not on internal FSM states.

Fog machine integration

Added digital output on pin 41 (FOG_MACHINE_PIN) to control a fog machine via external transistor.

Fog is triggered when the Bad Motivator reaches the TOP position (BMTop).

Activation is performed using edge detection (TOP transition), not level detection.

Fog remains active for 5 seconds and is then turned off automatically.

Fog is turned off early if the Bad Motivator starts moving down.

Non-blocking timing logic based on millis().

ESP32 periscope lightshow bridge

Added UART bridge to an external ESP32 periscope lightshow controller.

Uses hardware UART Serial3 on Arduino Mega 2560 at 115200 baud.

Added support for :PEQxx commands (range 0–20).

Commands received via Shadow/BetterDuino are forwarded as Qxx commands to the ESP32.

Fully compatible with existing MarcDuino-style command flow.

Changed
Command parser extension

Extended MarcDuino-like serial parser to recognize :PEQxx commands.

Added numeric parsing and range validation before forwarding commands to the ESP32.

Fog logic reliability

Fixed a logic issue where the fog machine could continuously rearm while the limit switch remained active.

Updated fog trigger logic from level-based to edge-based detection.

Added safe initialization of previous limit switch state at startup to avoid unintended activation after reset or power-up.

Fixed

Removed duplicate handling of the :L? command in the serial parser.

Prevented unintended fog machine activation on boot or reset.

Notes

All original DomeLift finite state machines and behaviors remain unchanged.

Motor control, servo control and button logic are untouched.

The modification is fully backward compatible with the original Printed-Droid DomeLift firmware.
