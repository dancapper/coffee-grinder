# coffee-grinder
Arduino code for a simple automatic coffee grinder

A one-shot timer which takes input as a potentiometer (10k and up, linear) and a single pushbutton switch.

This was designed to replace the busted electronics in my old russell hobbs automatic grinder but could be used for any similar application.

- The potentiometer sets the number of "cups" you wish to grind for; maximum is configurable; minimum is always one "cup".

- Short press of the button runs the output for a specified amount of time multiplied by the number of "cups".

- >1s press of the button allows you to set the amount of time per "cup"; press again to stop the output and the time will be remembered

- >3s press of the button will save the current timing setting to EEPROM.
