# sfml-chess-test

A graphical program using [CSFML] to interface with my own [chesslib](https://github.com/thearst3rd/chesslib). It is not very featureful and most commands are executed by keypresses, but it demonstrates the functionality of chesslib.

## Build

### Setup on Linux

This project depends on [CSFML], so install it for your distribution. On Ubuntu-based systems, you can install csfml with the following command:

```
sudo apt-get install libcsfml-dev
```

### Setup on Windows

The available makefile is compatible with [MSYS2](https://www.msys2.org/), so download it and install it. Make sure you have MinGW64 gcc and make installed, as well as csfml:

```
pacman -S mingw64/mingw-w64-x86_64-{gcc,make,libcsfml}
```

(TODO - make sure these are actually enough and the proper package names)

### Compile

To compile the application, you need both sfml-chess-test and chesslib. The following commands will checkout everything.

```
git clone https://github.com/thearst3rd/sfml-chess-test
git clone https://github.com/thearst3rd/chesslib
cd sfml-chess-test
make
```

To run the application, either type

```
make run
```

or directly run with

```
./bin/sfml-app
```

[CSFML]: https://www.sfml-dev.org/download/csfml/

## Usage instructions

You can make moves by clicking and dragging the pieces. If the move is legal, it will be played on the board.

The titlebar of the application will update with the FEN of the current board position. If the game has ended, the title will state how the game ended. Additionally, if the game is still ongoing, and the current position has been repeated more than once, the title will say how many times the current position has been seen.

This program has a "bot" which by default just makes random moves. More information in the table below. The two behaviors currently implemented are `aiRandomMove` which makes moves randomly, and `aiMinOpponentMoves` which makes a move which minimizes the number of moves with which the opponent can respond. The behavior of the bot can be changed by changing the behavior of the `aiGetMove` function.

The following keyboard commands can be used to interface with the program:

Key | Action
--- | ---
R | Restart the game
F | Flip the board perspective
A | Toggle auto-flip (the board will automatically flip to the perspective of the current player after each move)
H | Toggle square highlighting (showing the last move)
S | Toggle sound
M | Toggle the bot (which plays moves after you do)
Space | Have the bot play a move
Alt+Enter | Toggle fullscreen
G | Instantly play out the rest of the current game with bot moves. If the game was already ended, it'll reset the game and then play out and entire game using bot moves
Z | Undo the last move (if any). If a draw claim was made (threefold or 50 move rule), it will undo the draw claim
C | Claim a draw if available. It will first check if a draw by 50 move rule can be claimed, then it will check if draw by threefold repetition can be claimed
L | Toggle highlighting of legal squares when holding a piece
