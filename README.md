# Embedded Memory Game

A Simon-style memory game for the Arduino Uno, made for my Embedded Systems coursework (Assignment 2).

While idle, the four LEDs (red, green, yellow, blue) cycle until the start button is pressed. Each round the game plays a random sequence of colors with a matching buzzer tone, and you repeat it using the four color buttons. The sequence grows by one each round. Get one wrong and the game ends with a low buzz; make it through all 10 rounds to win and hear a victory arpeggio.

Built with PlatformIO. The wiring diagram is in `gameFritzing.fzz` / `gameFritzingScreenshot.png`.
