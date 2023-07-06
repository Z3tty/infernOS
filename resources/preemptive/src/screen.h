/*
 * Coordinates used by different threads used to print output.
 */

/* Clock thread */
enum
{
	CLOCK_LINE = 24,
	CLOCK_STR = 50,
	CLOCK_VALUE = 70
};

/* thread 2 and thread 3 */
enum
{
	LOCKTS_LINE = 10
};

/* Monte Carlo Pi */
enum
{
	MCPI_Y = 13, // Line where results are printed
	MCPI_X0 = 25,
	MCPI_X1 = 35,
	MCPI_X2 = 45,
	MCPI_X3 = 55,
	MCPI_FINAL = 65 // X coordinate
};

/* Barrier threads */
enum
{
	BARRIER_LINE = 11, /* Line where result should be printed */
	BARRIER_COL = 10   /* Column where result should be printed */
};

/* Plane process */
enum
{
	PLANE_ROWS = 4,
	PLANE_COLUMNS = 18,
	PLANE_LINE = 8,
	PLANE_PID_STR = 0,
	PLANE_PID = 8,
	PLANE_PRI_STR = 10,
	PLANE_PRI = 20
};

/* Math process */
enum
{
	MATH_LINE = 7
};

/* Philosophers */
enum
{
	PHIL_LINE = 11, /* Line where result should be printed */
	PHIL_COL = 0    /* Column where result should be printed */
};
