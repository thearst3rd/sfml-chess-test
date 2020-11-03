/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <SFML/Audio.h>

#include "chesslib/chessgame.h"

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define resources
sfRenderWindow *window;

sfRectangleShape *boardSquares[64];
sfColor boardBlackColor;
sfColor boardWhiteColor;
sfColor boardHighlightColor;
sfColor backgroundColor;
sfColor checkColor;
sfColor pieceTransparentColor;
sfColor legalMoveColor;

sfRectangleShape *highlightSquare;
sq highlight1Sq;
sq highlight2Sq;

sfCircleShape *checkIndicator;
sfCircleShape *legalMoveIndicator;
sfCircleShape *legalCaptureIndicator;

sfTexture *texWKing;
sfTexture *texWQueen;
sfTexture *texWRook;
sfTexture *texWBishop;
sfTexture *texWKnight;
sfTexture *texWPawn;

sfTexture *texBKing;
sfTexture *texBQueen;
sfTexture *texBRook;
sfTexture *texBBishop;
sfTexture *texBKnight;
sfTexture *texBPawn;

sfTexture *texPieces[12];

sfSprite *sprPiece;

sfSound *sndMove;
sfSound *sndCapture;
sfSound *sndCheck;
sfSound *sndTerminal;

// Define global variables
int isDragging = 0;
sq draggingSq;
piece newDraggingPiece;
int isFlipped = 0;
int autoFlip = 0;
int isFullscreen = 0;
int showHighlighting = 1;
int playSound = 1;
int doRandomMoves = 0;
int showLegals = 1;

const char *initialFen;
chessGame *g = NULL;


int main(int argc, char *argv[])
{
	// Set random seed
	srand(time(NULL));

	initialFen = INITIAL_FEN;

	// Parse command line input
	for (int i = 1; i < argc; i++)
	{
		if ((strcmp(argv[i], "--fen") == 0) || (strcmp(argv[i], "-f") == 0))
		{
			i++;
			if (i >= argc)
			{
				fprintf(stderr, "ERROR: You must supply FEN after the %s argument", argv[i - 1]);
				return 1;
			}
			initialFen = argv[i];
		}
	}

	// Create the window
	sfVideoMode mode = {720, 720, 32};
	// Default values taken from https://www.sfml-dev.org/documentation/2.5.1/structsf_1_1ContextSettings.php
	sfContextSettings contextSettings = (sfContextSettings)
	{
		0, // depth
		0, // stencil
		4, // antialiasing
		1, // major
		1, // minor
		sfContextDefault, // attributes
		sfFalse // sRgb
	};
	window = sfRenderWindow_create(mode, "SFML Chess Board", sfDefaultStyle, &contextSettings);
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
	texPieces[ 3] = texWBishop = sfTexture_createFromFile("img/wB.png", NULL);
	texPieces[ 4] = texWKnight = sfTexture_createFromFile("img/wN.png", NULL);
	texPieces[ 5] = texWPawn   = sfTexture_createFromFile("img/wP.png", NULL);

	texPieces[ 6] = texBKing   = sfTexture_createFromFile("img/bK.png", NULL);
	texPieces[ 7] = texBQueen  = sfTexture_createFromFile("img/bQ.png", NULL);
	texPieces[ 8] = texBRook   = sfTexture_createFromFile("img/bR.png", NULL);
	texPieces[ 9] = texBBishop = sfTexture_createFromFile("img/bB.png", NULL);
	texPieces[10] = texBKnight = sfTexture_createFromFile("img/bN.png", NULL);
	texPieces[11] = texBPawn   = sfTexture_createFromFile("img/bP.png", NULL);

	// Set window icon
	sfVector2u texSize = sfTexture_getSize(texBQueen);
	sfImage *iconImage = sfTexture_copyToImage(texBQueen);
	sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));

	// Set texture scaling
	for (int i = 0; i < 12; i++)
	{
		sfTexture_setSmooth(texPieces[i], sfTrue);
		sfTexture_generateMipmap(texPieces[i]);
	}

	// Create piece sprite. This will be reused for each piece drawing
	sprPiece = sfSprite_create();
	float size = (float) texSize.x; 	// Assumes square pieces, all the same size
	sfSprite_setScale(sprPiece, (sfVector2f) {SQUARE_SIZE / size, SQUARE_SIZE / size});
	sfSprite_setOrigin(sprPiece, (sfVector2f) {size / 2.0f, size / 2.0f});

	// Define board colors
	boardBlackColor = sfColor_fromRGB(167, 129, 177);
	boardWhiteColor = sfColor_fromRGB(234, 223, 237);
	boardHighlightColor = sfColor_fromRGBA(255, 255, 0, 100);
	backgroundColor = sfColor_fromRGB(25, 25, 25);
	checkColor = sfColor_fromRGBA(255, 0, 0, 100);
	pieceTransparentColor = sfColor_fromRGBA(255, 255, 255, 70);
	legalMoveColor = sfColor_fromRGBA(0, 0, 0, 100);

	// Create board squares
	for (int i = 0; i < 64; i++)
	{
		sfRectangleShape *s = sfRectangleShape_create();

		sfRectangleShape_setSize(s, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
		sfRectangleShape_setPosition(s, (sfVector2f) {(float) (i % 8) * SQUARE_SIZE, floor(i / 8) * SQUARE_SIZE});

		int isBlack = (i + (int) floor(i / 8)) % 2;
		sfRectangleShape_setFillColor(s, isBlack ? boardBlackColor : boardWhiteColor);

		boardSquares[i] = s;
	}

	// Create highlight square
	highlightSquare = sfRectangleShape_create();

	sfRectangleShape_setSize(highlightSquare, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
	sfRectangleShape_setFillColor(highlightSquare, boardHighlightColor);

	// Create check indicator
	checkIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(checkIndicator, SQUARE_SIZE / 2.1f);
	sfCircleShape_setOrigin(checkIndicator, (sfVector2f) {SQUARE_SIZE / 2.1f, SQUARE_SIZE / 2.1f});
	sfCircleShape_setFillColor(checkIndicator, checkColor);

	// Create legal move indicator
	legalMoveIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(legalMoveIndicator, SQUARE_SIZE / 5.0f);
	sfCircleShape_setOrigin(legalMoveIndicator, (sfVector2f) {SQUARE_SIZE / 5.0f, SQUARE_SIZE / 5.0f});
	sfCircleShape_setFillColor(legalMoveIndicator, legalMoveColor);

	legalCaptureIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(legalCaptureIndicator, SQUARE_SIZE / 2.2f);
	sfCircleShape_setOrigin(legalCaptureIndicator, (sfVector2f) {SQUARE_SIZE / 2.2f, SQUARE_SIZE / 2.2f});
	sfCircleShape_setFillColor(legalCaptureIndicator, legalMoveColor);

	calcView();

	initChessGame();

	// Create sounds
	sfSoundBuffer *sbMove = sfSoundBuffer_createFromFile("snd/move.wav");
	sfSoundBuffer *sbCapture = sfSoundBuffer_createFromFile("snd/capture.wav");
	sfSoundBuffer *sbCheck = sfSoundBuffer_createFromFile("snd/check.wav");
	sfSoundBuffer *sbTerminal = sfSoundBuffer_createFromFile("snd/terminal.wav");

	sndMove = sfSound_create();
	sfSound_setBuffer(sndMove, sbMove);
	sndCapture = sfSound_create();
	sfSound_setBuffer(sndCapture, sbCapture);
	sndCheck = sfSound_create();
	sfSound_setBuffer(sndCheck, sbCheck);
	sndTerminal = sfSound_create();
	sfSound_setBuffer(sndTerminal, sbTerminal);


	// Main loop
	while (sfRenderWindow_isOpen(window))
	{
		board *b = chessGameGetCurrentBoard(g);

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
				calcView();
			}
			else if (event.type == sfEvtMouseButtonPressed)
			{
				if (event.mouseButton.button == sfMouseLeft)
				{
					sq s;
					piece p;
					if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &s))
					{
						if (g->terminal == tsOngoing)
						{
							p = boardGetPiece(b, s);

							if (p && (pieceGetColor(p) == b->currentPlayer))
							{
								isDragging = 1;
								draggingSq = s;
								newDraggingPiece = pEmpty;
							}
						}
					}
				}
			}
			else if (event.type == sfEvtMouseButtonReleased)
			{
				if (event.mouseButton.button == sfMouseLeft)
				{
					if (isDragging)
					{
						isDragging = 0;

						sq s;
						if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &s))
						{
							move m = moveSq(draggingSq, s);
							if (pieceGetType(boardGetPiece(b, draggingSq)) == ptPawn && (s.rank == 1 || s.rank == 8))
								m.promotion = ptQueen;

							uint8_t isCapture = (boardGetPiece(b, s) != pEmpty) ||
									(pieceGetType(boardGetPiece(b, draggingSq)) == ptPawn && (s.file != draggingSq.file));

							if (!chessGamePlayMove(g, m))
							{
								b = chessGameGetCurrentBoard(g);

								uint8_t isCheck = boardIsInCheck(b) && (g->terminal == tsOngoing);

								if (doRandomMoves && (g->terminal == tsOngoing))
								{
									playAiMove();

									move aiMove = g->moveHistory->tail->move;

									isCapture |= boardGetPiece(b, aiMove.to) ||
											(pieceGetType(boardGetPiece(b, aiMove.from)) == ptPawn
											&& aiMove.to.file != aiMove.from.file);

									b = chessGameGetCurrentBoard(g);

									isCheck |= boardIsInCheck(b);
								}

								if (playSound)
								{
									if (isCapture)
										sfSound_play(sndCapture);
									else
										sfSound_play(sndMove);

									if (g->terminal != tsOngoing)
										sfSound_play(sndTerminal);
									else if (isCheck)
										sfSound_play(sndCheck);
								}

								updateGameState();
							}
						}
					}
				}
			}
			else if (event.type == sfEvtKeyPressed)
			{
				switch (event.key.code)
				{
					case sfKeyR:
						if (isDragging)
							break;
						initChessGame();
						b = chessGameGetCurrentBoard(g);
						break;

					case sfKeyF:
						isFlipped = !isFlipped;
						break;

					case sfKeyA:
						autoFlip = !autoFlip;
						break;

					case sfKeyH:
						showHighlighting = !showHighlighting;
						break;

					case sfKeyS:
						playSound = !playSound;
						break;

					case sfKeyM:
						doRandomMoves = !doRandomMoves;
						break;

					case sfKeySpace:
						if (isDragging)
							break;
						if (g->terminal == tsOngoing)
						{
							playAiMove();

							move aiMove = g->moveHistory->tail->move;

							uint8_t isCapture = boardGetPiece(b, aiMove.to) ||
									(pieceGetType(boardGetPiece(b, aiMove.from)) == ptPawn
									&& aiMove.to.file != aiMove.from.file);

							b = chessGameGetCurrentBoard(g);

							if (playSound)
							{
								if (isCapture)
									sfSound_play(sndCapture);
								else
									sfSound_play(sndMove);

								if (g->terminal != tsOngoing)
									sfSound_play(sndTerminal);
								else if (boardIsInCheck(b))
									sfSound_play(sndCheck);
							}

							updateGameState();
						}
						break;

					case sfKeyEnter:
						if (event.key.alt)
						{
							isFullscreen = !isFullscreen;
							sfRenderWindow_destroy(window);

							sfVideoMode videoMode = isFullscreen ? sfVideoMode_getDesktopMode() : mode;
							sfWindowStyle style = isFullscreen ? sfFullscreen : sfDefaultStyle;

							char *fen = boardGetFen(b);

							window = sfRenderWindow_create(videoMode, fen, style, NULL);

							free(fen);

							sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));
							sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

							calcView();
						}
						break;

					case sfKeyG:
						if (isDragging)
							break;
						if (g->terminal != tsOngoing)
							initChessGame();
						while (g->terminal == tsOngoing)
						{
							moveList *list = g->currentLegalMoves;

							int index = rand() % list->size;
							moveListNode *n = list->head;
							for (int i = 0; i < index; i++)
								n = n->next;
							move m = n->move;
							chessGamePlayMove(g, m);
						}
						b = chessGameGetCurrentBoard(g);

						updateGameState();
						break;

					case sfKeyZ:
						if (isDragging)
							break;
						chessGameUndo(g);
						b = chessGameGetCurrentBoard(g);

						updateGameState();
						break;

					case sfKeyC:
						if (isDragging)
							break;
						if (g->terminal == tsOngoing)
						{
							if (g->repetitions >= 3)
							{
								g->terminal = tsDrawClaimedThreefold;
								if (playSound)
									sfSound_play(sndTerminal);
							}
							else if (b->halfMoveClock >= 100)
							{
								g->terminal = tsDrawClaimed50MoveRule;
								if (playSound)
									sfSound_play(sndTerminal);
							}
						}
						break;

					case sfKeyL:
						showLegals = !showLegals;
						break;

					default:
						break;
				}
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, backgroundColor);

		// Draw the board
		for (int i = 0; i < 64; i++)
		{
			sq s;
			s.file = (i % 8) + 1;
			s.rank = 8 - floor(i / 8);

			int index = isFlipped ? 63 - i : i;

			sfRenderWindow_drawRectangleShape(window, boardSquares[index], NULL);

			if (showHighlighting && (sqEq(s, highlight1Sq) || sqEq(s, highlight2Sq)))
			{
				sfRectangleShape_setPosition(highlightSquare, sfRectangleShape_getPosition(boardSquares[index]));
				sfRenderWindow_drawRectangleShape(window, highlightSquare, NULL);
			}

			piece p = boardGetPiece(b, s);
			if (p)
			{
				pieceType pt = pieceGetType(p);

				if (pt == ptKing && boardIsSquareAttacked(b, s, b->currentPlayer == pcWhite ? pcBlack : pcWhite))
					drawCircleShape(checkIndicator, s);
				drawBoardPiece(p, s);
			}

			if (showLegals && isDragging)
			{
				for (moveListNode *n = g->currentLegalMoves->head; n; n = n->next)
				{
					if (sqEq(n->move.from, draggingSq) && sqEq(n->move.to, s))
					{
						drawCircleShape(p ? legalCaptureIndicator : legalMoveIndicator, s);
						break;
					}
				}
			}
		}

		// Draw currently dragged piece
		if (isDragging)
		{
			piece p;
			if (newDraggingPiece)
				p = newDraggingPiece;
			else
				p = boardGetPiece(b, draggingSq);

			if (p)
			{
				sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, sfMouse_getPosition((sfWindow *) window), NULL);

				drawPiece(p, coords);
			}
		}

		sfRenderWindow_display(window);
	}

	// Cleanup and exit
	sfRenderWindow_destroy(window);

	for (int i = 0; i < 64; i++)
		sfRectangleShape_destroy(boardSquares[i]);

	for (int i = 0; i < 12; i++)
		sfTexture_destroy(texPieces[i]);

	sfRectangleShape_destroy(highlightSquare);
	sfCircleShape_destroy(checkIndicator);
	sfCircleShape_destroy(legalMoveIndicator);
	sfCircleShape_destroy(legalCaptureIndicator);

	sfSprite_destroy(sprPiece);

	sfSoundBuffer_destroy(sbMove);
	sfSoundBuffer_destroy(sbCapture);
	sfSoundBuffer_destroy(sbCheck);
	sfSoundBuffer_destroy(sbTerminal);

	sfSound_destroy(sndMove);
	sfSound_destroy(sndCapture);
	sfSound_destroy(sndCheck);
	sfSound_destroy(sndTerminal);

	return 0;
}

void calcView()
{
	sfVector2u windowSize = sfRenderWindow_getSize(window);
	float width = (float) windowSize.x;
	float height = (float) windowSize.y;

	float x = 0.0f;
	float y = 0.0f;
	float w = SQUARE_SIZE * 8.0f;
	float h = SQUARE_SIZE * 8.0f;

	float ratio = (w / h);
	float windowRatio = (width / height);

	if (windowRatio < ratio)
	{
		float yDiff = w * ((1.0f / windowRatio) - (1.0f / ratio));
		h += yDiff;
		y -= yDiff / 2.0f;
	}
	else
	{
		float xDiff = h * (windowRatio - ratio);
		w += xDiff;
		x -= xDiff / 2.0f;
	}

	sfView *newView = sfView_createFromRect((sfFloatRect) {x, y, w, h});
	sfRenderWindow_setView(window, newView);
	sfView_destroy(newView);
}

void drawPiece(piece p, sfVector2f coords)
{
	sfTexture *tex = getPieceTex(p);

	sfSprite_setTexture(sprPiece, tex, sfFalse);
	sfSprite_setPosition(sprPiece, coords);
	sfRenderWindow_drawSprite(window, sprPiece, NULL);
}

void drawBoardPiece(piece p, sq s)
{
	int coordsFile, coordsRank;

	if (isFlipped)
	{
		coordsFile = 9 - s.file;
		coordsRank = 9 - s.rank;
	}
	else
	{
		coordsFile = s.file;
		coordsRank = s.rank;
	}

	sfVector2f coords = (sfVector2f) {((float) coordsFile - 0.5) * SQUARE_SIZE,
			(8.5 - (float) coordsRank) * SQUARE_SIZE};

	if (isDragging && sqEq(draggingSq, s))
	{
		sfSprite_setColor(sprPiece, pieceTransparentColor);
		drawPiece(p, coords);
		sfSprite_setColor(sprPiece, sfWhite);
	}
	else
	{
		drawPiece(p, coords);
	}
}

void drawCircleShape(sfCircleShape *shape, sq s)
{
	uint8_t file = s.file;
	uint8_t rank = s.rank;

	if (isFlipped)
	{
		file = 9 - file;
		rank = 9 - rank;
	}

	sfCircleShape_setPosition(shape, (sfVector2f) {((float) file - 0.5) * SQUARE_SIZE, (8.5 - (float) rank) * SQUARE_SIZE});
	sfRenderWindow_drawCircleShape(window, shape, NULL);
}

sfTexture *getPieceTex(piece p)
{
	switch (p)
	{
		case pWPawn:
			return texWPawn;
		case pWKnight:
			return texWKnight;
		case pWBishop:
			return texWBishop;
		case pWRook:
			return texWRook;
		case pWQueen:
			return texWQueen;
		case pWKing:
			return texWKing;
		case pBPawn:
			return texBPawn;
		case pBKnight:
			return texBKnight;
		case pBBishop:
			return texBBishop;
		case pBRook:
			return texBRook;
		case pBQueen:
			return texBQueen;
		case pBKing:
			return texBKing;
		default:
			return NULL;
	}
}

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, sq *s)
{
	sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, (sfVector2i) {mouseX, mouseY}, NULL);
	coords.x /= SQUARE_SIZE;
	coords.y /= SQUARE_SIZE;

	if (coords.x < 0.0f || coords.x >= 8.0f || coords.y < 0.0f || coords.y >= 8.0f)
		return 0;

	s->file = 1 + (int) floor(coords.x);
	s->rank = 8 - (int) floor(coords.y);

	if (isFlipped)
	{
		s->file = 9 - s->file;
		s->rank = 9 - s->rank;
	}

	return 1;
}

void initChessGame()
{
	if (g)
		chessGameFree(g);

	g = chessGameCreateFromFen(initialFen);

	updateGameState();
}

void updateGameState()
{
	char *fen = boardGetFen(chessGameGetCurrentBoard(g));
	sfRenderWindow_setTitle(window, fen);
	free(fen);

	if (g->moveHistory->tail == NULL)
	{
		highlight1Sq = SQ_INVALID;
		highlight2Sq = SQ_INVALID;
	}
	else
	{
		move m = g->moveHistory->tail->move;

		highlight1Sq = m.from;
		highlight2Sq = m.to;
	}

	if (g->terminal == tsOngoing)
	{
		if (autoFlip)
			isFlipped = chessGameGetCurrentBoard(g)->currentPlayer == pcBlack;
	}
}

void playAiMove()
{
	if (g->terminal != tsOngoing)
		return;

	moveList *list = g->currentLegalMoves;
	uint8_t index = rand() % list->size;

	moveListNode *n = list->head;
	for (int i = 0; i < index; i++)
	{
		n = n->next;
	}

	move m = n->move;

	chessGamePlayMove(g, m);
}
