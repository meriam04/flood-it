# Play Flood-it on CPUlator

> Use the DE1-SoC simulator here: https://cpulator.01xz.net/?sys=arm-de1soc

## 1.0 Beginning the game

Before beginning a game, users can select the level they wish to play using the switches. To confirm the level and start the game, users can press and release any pushbutton.

The switch selected corresponds to the level, as outlined below:

- 0 (Test round)
- 1 (Easy)
- 2 (Intermediate)
- 3 (Hard)
- 4 (Very Hard)

Note: If none of the above options are selected, the default level is Level 2 (intermediate). Furthermore, if multiple of the above switches are selected, the program takes the lowest selected level. 

Users can start a new game at any time in the same manner.

## 2.0 Playing the game

The game is designed so that when the user selects a colour, the top left square becomes that colour and connects to other neighbouring boxes of the same colour. As the user continues to play, the group of tiles in the top left continues to grow, until they fill the entire grid with the same colour or run out of moves.

To change the colour of the top left tile, users can press the “enter” key on the selected tile (outlined tile), and the flood colour will become the colour of the selected tile. To change the selected tile, press the arrow keys in the appropriate directions. 

The number of remaining moves is displayed on the HEX display. When the game is over, a “passed” or “failed” message is displayed in the hex display, and a “game over screen” is displayed on the VGA controller.

## 3.0 Starting a new game

Users can use the same steps to start a new game as they did in the beginning.

## 4.0 Demo

Check out a demo of the game!

https://github.com/meriam04/flood-it/assets/81719754/31b613df-a556-44c6-a18d-022cfa8c6348

