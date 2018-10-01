# Servoless_retract_control
Used to control a servoless retract module for a drone using a BMP180 barometric pressure sensor
to measure altitude.  Settings based on the altitude will tell when the landing gear will extend 
or retract.

This is the menu for the firmware showing all options.  This can be viewed when connecting to the
module using the terminal in mission planner by typing "menu".
-------------------------------------------------------------------------------------------------
---=== Debug Menu ===---
Toggle options:
  aon/aoff - toggle displaying altitude information
  ton/toff - toggle displaying temperature information
  pon/poff - toggle displaying pressure information
  allon/alloff - toggle displaying all 3

Eeprom settings:

  alt: - Set altitude height in inches to trigger the landing gear. (Only use 60-254 no decimal)
         Below this height the gear will extend.  Above this height gear will retract.  Sending
         "alt:" with no height will show the current trigger height.
  Example:
  alt:72  (72 inches is 6 feet)

  servo: - Sets the servos low and high settings to extend and retract
  Example:
  servo:1000:2000  (extends at 1000, retracts at 2000)

Control:

  extend - Forces the gear to etend.  This is to help release the retracts if stuck in
           a certain position.

  retract - Forces the gear to retract.  This is to help release the retracts if stuck
            in a certain position.  
  Depending on the current height of the drone in relation to the baseline, retract and/or extend
  might not have any affect.
  
-------------------------------------------------------------------------------------------------
