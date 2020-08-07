/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <math.h>

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define global variables
sfRenderWindow *window;

sfRectangleShape *boardSquares[64];
sfColor boardBlackColor;
sfColor boardWhiteColor;

sfTexture *texWKing;
sfTexture *texWQueen;
sfTexture *texWRook;
sfTexture *texWKnight;
sfTexture *texWBishop;
sfTexture *texWPawn;

sfTexture *texBKing;
sfTexture *texBQueen;
sfTexture *texBRook;
sfTexture *texBKnight;
sfTexture *texBBishop;
sfTexture *texBPawn;

sfTexture *texPieces[12];

sfSprite *sprPiece;

int main(int argc, char *argv[])
{
	// Create the window
	window = sfRenderWindow_create((sfVideoMode) {800, 600, 32}, "SFML Chess Board", sfResize | sfClose, NULL);
	if (!window)
	{
		fprintf(stderr, "ERROR: Unable to create SFML window");
		return 1;
	}

	sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

	// Load textures
	texPieces[ 0] = texWKing   = sfTexture_createFromFile("img/wK.png", NULL);
	texPieces[ 1] = texWQueen  = sfTexture_createFromFile("img/wQ.png", NULL);
	texPieces[ 2] = texWRook   = sfTexture_createFromFile("img/wR.png", NULL);
	texPieces[ 3] = texWKnight = sfTexture_createFromFile("img/wN.png", NULL);
	texPieces[ 4] = texWBishop = sfTexture_createFromFile("img/wB.png", NULL);
	texPieces[ 5] = texWPawn   = sfTexture_createFromFile("img/wP.png", NULL);

	texPieces[ 6] = texBKing   = sfTexture_createFromFile("img/bK.png", NULL);
	texPieces[ 7] = texBQueen  = sfTexture_createFromFile("img/bQ.png", NULL);
	texPieces[ 8] = texBRook   = sfTexture_createFromFile("img/bR.png", NULL);
	texPieces[ 9] = texBKnight = sfTexture_createFromFile("img/bN.png", NULL);
	texPieces[10] = texBBishop = sfTexture_createFromFile("img/bB.png", NULL);
	texPieces[11] = texBPawn   = sfTexture_createFromFile("img/bP.png", NULL);

	// Set texture scaling
	for (int i = 0; i < 12; i++)
	{
		sfTexture_setSmooth(texPieces[i], sfTrue);
	}

	// Create piece sprite. This will be reused for each piece drawing
	sprPiece = sfSprite_create();
	sfSprite_setScale(sprPiece, (sfVector2f) {SQUARE_SIZE / 240.0f, SQUARE_SIZE / 240.0f });

	// Define board colors
	boardBlackColor = sfColor_fromRGB(151, 124, 179);
	boardWhiteColor = sfColor_fromRGB(227, 216, 238);

	// Create board
	for (int i = 0; i < 64; i++)
	{
		sfRectangleShape *s = sfRectangleShape_create();

		sfRectangleShape_setSize(s, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
		sfRectangleShape_setPosition(s, (sfVector2f) {(float) (i % 8) * SQUARE_SIZE, floor(i / 8) * SQUARE_SIZE});

		int isBlack = !((i + (int) floor(i / 8)) % 2); 	// For now. Will fix this
		sfRectangleShape_setFillColor(s, isBlack ? boardBlackColor : boardWhiteColor);

		boardSquares[i] = s;
	}

	calcView(800.0f, 600.0f);

	// Main loop
	while (sfRenderWindow_isOpen(window))
	{
		// Handle events
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event))
		{
			if (event.type == sfEvtClosed)
			{
				sfRenderWindow_close(window);
			}
			else if (event.type == sfEvtResized)
			{
				calcView((float) event.size.width, (float) event.size.height);
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, sfBlack);

		// Draw the board
		for (int i = 0; i < 64; i++)
			sfRenderWindow_drawRectangleShape(window, boardSquares[i], NULL);

		// Draw pieces
		drawPiece(texWPawn,   1, 2);
		drawPiece(texWPawn,   2, 2);
		drawPiece(texWPawn,   3, 2);
		drawPiece(texWPawn,   4, 2);
		drawPiece(texWPawn,   5, 4);
		drawPiece(texWPawn,   6, 2);
		drawPiece(texWPawn,   7, 2);
		drawPiece(texWPawn,   8, 2);
		drawPiece(texWRook,   1, 1);
		drawPiece(texWKnight, 2, 1);
		drawPiece(texWBishop, 3, 1);
		drawPiece(texWQueen,  6, 7);
		drawPiece(texWKing,   5, 2); 	// Ke2 PogChamp
		drawPiece(texWBishop, 3, 4);
		drawPiece(texWKnight, 7, 1);
		drawPiece(texWRook,   8, 1);

		drawPiece(texBPawn,   1, 7);
		drawPiece(texBPawn,   2, 7);
		drawPiece(texBPawn,   3, 7);
		drawPiece(texBPawn,   4, 7);
		drawPiece(texBPawn,   5, 5);
		//drawPiece(texBPawn,   6, 7);
		drawPiece(texBPawn,   7, 7);
		drawPiece(texBPawn,   8, 7);
		drawPiece(texBRook,   1, 8);
		drawPiece(texBKnight, 3, 6);
		drawPiece(texBBishop, 3, 8);
		drawPiece(texBQueen,  4, 8);
		drawPiece(texBKing,   5, 8);
		drawPiece(texBBishop, 6, 8);
		drawPiece(texBKnight, 6, 6);
		drawPiece(texBRook,   8, 8);

		sfRenderWindow_display(window);
	}

	// Cleanup and exit
	sfRenderWindow_destroy(window);

	for (int i = 0; i < 64; i++)
		sfRectangleShape_destroy(boardSquares[i]);

	for (int i = 0; i < 12; i++)
		sfTexture_destroy(texPieces[i]);

	return 0;
}

void calcView(float width, float height)
{
	float x = 0.0f;
	float y = 0.0f;
	float w = SQUARE_SIZE * 8.0f;
	float h = SQUARE_SIZE * 8.0f;

	if (height > width)
	{
		y = (h * (height/width) - h);
		h += y;
		y /= -2.0f;
	}
	else
	{
		x = (w * (width/height) - w);
		w += x;
		x /= -2.0f;
	}

	sfView *newView = sfView_createFromRect((sfFloatRect) {x, y, w, h});
	sfRenderWindow_setView(window, newView);
	sfView_destroy(newView);
}

void drawPiece(sfTexture *tex, int file, int rank)
{
	sfSprite_setTexture(sprPiece, tex, NULL);
	sfSprite_setPosition(sprPiece, (sfVector2f) {(file - 1) * SQUARE_SIZE, (8 - rank) * SQUARE_SIZE});
	sfRenderWindow_drawSprite(window, sprPiece, NULL);
}
