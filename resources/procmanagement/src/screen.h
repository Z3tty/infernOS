/*
 * Coordinates used by different threads used to print output.
 */

#ifndef SCREEN_H
#define SCREEN_H

/* Clock thread */
enum
{
	CLOCK_BASE_LINE = 17
	// In addition lines base - 5 through base + #runnable are used
};

/* thread 2 and thread 3 */
enum
{
	LOCKTS_LINE = 10
};

/* Shell process - defines the shell window */
enum
{
	SHELL_MINX = 30,
	SHELL_SIZEX = 50,
	SHELL_MINY = 0,
	SHELL_SIZEY = 7,
	MAXX = SHELL_MINX + SHELL_SIZEX,
	MAXY = SHELL_MINY + SHELL_SIZEY,
	SHELL_HISTY = 100
};

/* Plane process */
enum
{
	PLANE_ROWS = 4,
	PLANE_COLUMNS = 20,
	PLANE_ERROR_Y = 7,
	PLANE_ERROR_X = 30,
	PLANE_LOC_X_MIN = 10,
	PLANE_LOC_X_MAX = 80,
	PLANE_LOC_Y_MAX = 7,
	PLANE_BULLET_X_MIN = 30
};

/* Math process */
enum
{
	MATH_LINE = 7
};

/* Message passing processes */
enum
{
	PROC3_LINE = 4,
	PROC4_LINE = 4
};

#endif
