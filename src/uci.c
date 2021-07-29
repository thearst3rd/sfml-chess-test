// UCI engine support

#include "uci.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

FILE *engineWrite; 	// File to write data into engine
FILE *engineRead; 	// File to read data back from engine

pid_t p;

char *uciInitialFen = NULL;
char *uciLimit = NULL;


void uciCreate(const char *execName)
{
	int fdTo[2]; 	// Pipe to write data into subprocess
	int fdFrom[2]; 	// Pipe to read data from subprocess

	if (pipe(fdTo) == -1)
	{
		fprintf(stderr, "Pipe 1 failed\n");
		exit(EXIT_FAILURE);
	}
	if (pipe(fdFrom) == -1)
	{
		fprintf(stderr, "Pipe 2 failed\n");
		exit(EXIT_FAILURE);
	}

	printf("Spawning engine process...\n");
	p = fork();
	if (p < 0)
	{
		fprintf(stderr, "Fork failed\n");
		exit(EXIT_FAILURE);
	}

	if (p > 0) 	// PARENT PROCESS
	{
		// Close ends of pipe we don't care about
		close(fdTo[0]);
		close(fdFrom[1]);

		// Open easier-to-use files from the file descriptor (man I hope I can do this)
		engineWrite = fdopen(fdTo[1], "w");
		if (engineWrite == NULL)
		{
			fprintf(stderr, "engineWrite FILE* is null\n");
			exit(EXIT_FAILURE);
		}
		engineRead = fdopen(fdFrom[0], "r");
		if (engineRead == NULL)
		{
			fprintf(stderr, "engineRead FILE* is null\n");
			exit(EXIT_FAILURE);
		}

		// Initialize engine
		printf("Initializing UCI engine...\n");
		fprintf(engineWrite, "uci\n");
		fflush(engineWrite);
		if (ferror(engineWrite))
		{
			fprintf(stderr, "Failure to write data\n");
			exit(EXIT_FAILURE);
		}

		char *line = NULL;
		size_t len = 0;
		ssize_t read;

		while (1)
		{
			read = getline(&line, &len, engineRead);
			if (read != -1)
			{
				if (strcmp(line, "uciok\n") == 0)
				{
					printf("UCI ok!\n");
					return;
				}
			}
		}
		free(line);
	}
	else 	// CHILD PROCESS
	{
		close(fdTo[1]);
		close(fdFrom[0]);
		dup2(fdTo[0], 0);
		dup2(fdFrom[1], 1);
		execlp(execName, execName, (char *) NULL);
		fprintf(stderr, "execlp failed\n");
		exit(1);
	}
}

void uciFree()
{
	fprintf(engineWrite, "quit\n");
	fflush(engineWrite);
	fclose(engineWrite);
	fclose(engineRead);
	wait(NULL);
	printf("UCI engine closed\n");
}

void uciSetInitialFen(const char *newInitialFen)
{
	if (uciInitialFen)
		free(uciInitialFen);

	if (newInitialFen)
		uciInitialFen = strdup(newInitialFen);
	else
		uciInitialFen = NULL;
}

void uciSetLimit(const char *newLimit)
{
	if (uciLimit)
		free(uciLimit);

	if (newLimit)
		uciLimit = strdup(newLimit);
	else
		uciLimit = NULL;
}

void uciSetOption(const char *name, const char *value)
{
	fprintf(engineWrite, "setoption name %s value %s\n", name, value);
	fflush(engineWrite);
}

// Creates a concated string of the list of UCI moves. Must be freed
char *uciCreateMoveString(chess *game)
{
	moveList *history = chessGetMoveHistory(game);
	char *buf;

	if (history->size == 0)
	{
		buf = (char *) malloc(1);
		buf[0] = 0;
		return buf;
	}
	else if (history->size == 1)
	{
		return moveGetUci(history->head->move);
	}

	buf = (char *) malloc((6 * history->size) + 1);

	char *ptr = buf;

	for (moveListNode *n = history->head; n; n = n->next)
	{
		char *moveStr = moveGetUci(n->move);
		size_t len = strlen(moveStr);
		strncpy(ptr, moveStr, len);
		free(moveStr);

		ptr += len;

		if (n == history->tail)
		{
			ptr[0] = 0;
		}
		else
		{
			ptr[0] = ' ';
			ptr++;
		}
	}

	return buf;
}

move uciGetMove(chess *game)
{
	char *moveString = uciCreateMoveString(game);

	// Send over position
	if (uciInitialFen)
		fprintf(engineWrite, "position fen %s moves %s\n", uciInitialFen, moveString);
	else
		fprintf(engineWrite, "position startpos moves %s\n", moveString);

	if (uciLimit)
		fprintf(engineWrite, "go %s\n", uciLimit);
	else
		fprintf(engineWrite, "go depth 15\n");

	fflush(engineWrite);

	free(moveString);

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	while (1)
	{
		read = getline(&line, &len, engineRead);
		if (read != -1)
		{
			if (len > 8 && (strncmp(line, "bestmove ", 9) == 0))
			{
				char moveStr[6];
				strncpy(moveStr, line + 9, 5);
				moveStr[5] = 0;
				if ((moveStr[4] != 'q') && (moveStr[4] != 'n') && (moveStr[4] != 'r') && (moveStr[4] != 'b'))
					moveStr[4] = 0;

				printf("Best move received: %s\n", moveStr);
				move m = moveFromUci(moveStr);
				return m;
			}
		}
	}
	free(line);
}
