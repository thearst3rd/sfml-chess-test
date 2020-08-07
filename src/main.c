/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <math.h>
#include <SFML/Graphics.h>
#include <SFML/System.h>

#include "main.h"

// Define global variables
sfRectangleShape *boardSquares[64];
sfRenderWindow *window;

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

	// Create board
	for (int i = 0; i < 64; i++)
	{
		sfRectangleShape *s = sfRectangleShape_create();

		int isBlack = !((i + (int) floor(i / 8)) % 2); 	// For now. Will fix this
		sfRectangleShape_setFillColor(s, isBlack ? sfGreen : sfBlue);

		boardSquares[i] = s;
	}

	resizeBoard(800.0f, 600.0f);

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
				sfView *newView = sfView_createFromRect((sfFloatRect) {0, 0, event.size.width, event.size.height});
				sfRenderWindow_setView(window, newView);
				sfView_destroy(newView);

				resizeBoard((float) event.size.width, (float) event.size.height);
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, sfBlack);

		// Draw the board
		for (int i = 0; i < 64; i++)
			sfRenderWindow_drawRectangleShape(window, boardSquares[i], NULL);

		sfRenderWindow_display(window);
	}

	// Cleanup and exit
	sfRenderWindow_destroy(window);
	for (int i = 0; i < 64; i++)
		sfRectangleShape_destroy(boardSquares[i]);
	return 0;
}

void resizeBoard(float width, float height)
{
	// Calculate square size and offset
	float squareSize;
	float xOff;
	float yOff;

	if (height > width)
		squareSize = floor(width / 8.0f);
	else
		squareSize = floor(height / 8.0f);
	xOff = floor((width - 8.0f * squareSize) / 2.0f);
	yOff = floor((height - 8.0f * squareSize) / 2.0f);

	for (int i = 0; i < 64; i++)
	{
		sfRectangleShape *s = boardSquares[i];

		sfRectangleShape_setSize(s, (sfVector2f) {squareSize, squareSize});
		sfRectangleShape_setPosition(s, (sfVector2f) {(float) (i % 8) * squareSize + xOff, floor(i / 8) * squareSize + yOff});
	}
}
