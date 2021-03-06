/*
 * Options for the program. We don't want any magic numbers.
 */

#ifndef H_OPTIONS
#define H_OPTIONS

#include <string>
#include <vector>

#include "data.h"

// Avoid compiler warnings
typedef std::vector<int>::size_type   vsize_int;
typedef std::vector<Coord>::size_type vsize_coord;

// Allow easy debugging, enable debug mode with "-d" command-line option. When
// enabled we will save debug0.png, debug1.png, etc. for the PDF pages processed
// with marks of size MARK_SIZE and color MARK_COLOR.
extern bool DEBUG;
static const int MARK_SIZE = 5;
static const unsigned char MARK_COLOR = 127;

// Whether to write to a log file, and the file we will write to if true. If
// enabled, the log messages written to the screen in debug mode will be
// written to this file.
static const bool LOGGING = true;
static const std::string LOG_FILE = "freetron.log";

// The aspect ratio of the black boxes calculated from 49/18, width/height.
// This is used to verify that we have a valid box.
static const double ASPECT = 2.722;

// Minimum percent of pixels in a supposed box needing to be black to be
// considered a box. This is between 0 and 1.
static const double MIN_BLACK = 0.7;

// Maximum percent of pixels that can be black in the region around a box, and
// what sized region around box to check in pixels.
static const double MAX_BLACK = 0.5;
static const int WHITE_SEARCH = 5;

// What is considered black initially when auto thresholding in histogram.h. A
// value of 165 works best if we didn't have any auto thresholding, but since
// we do, it'll pick something good if this is somewhere near 50% of 256.
static const int GRAY_SHADE = 127;

// When finding the filled in bubbles, look for something as wide as a box with
// a height calculated from the below aspect ratio. The curve of the bubble is
// generated from a quarter-circle with it's center BUBBLE_CURVE*width pixels
// in on each side. Note that searching for the bubbles also uses WHITE_SEARCH
// to look at the surrounding region.
static const double BUBBLE_ASPECT = 1.566; // From 47/30

// The error margin in pixels for still considering this form as potentially
// valid.  After we find all boxes, we'll look for the filled in bubbles. If
// the boxes are beyond this far from vertical, we'll give up and say an error
// occurred while processing this form.
static const int MAX_ERROR = 5;

// The error margin in pixels for difference in height from the estimated
// height from the aspect ratio and width. If it's beyond this it won't be
// considered a box.
static const int HEIGHT_ERROR = 5;

// The max distance a point on the rectangle's side can be from the straight
// line connecting the two corners. This is relative to the width.
static const double RECT_ERROR = 0.125; // 4/32 = 0.125

// The error margin in pixels for the difference in diagonal from the diagonals
// of other valid boxes.
static const int DIAG_ERROR = 10;

// The error margin for the difference in slope for the width and height. This
// is to make sure that it is more of a parallelogram instead of just a
// quadrilateral.
static const double SLOPE_ERROR_WIDTH  = 0.2;  // 5/50 is 0.1,  10/50 is 0.2
static const double SLOPE_ERROR_HEIGHT = 0.5;  // 5/20 is 0.25, 10/20 is 0.5

// These values are used to make sure we'll get decent sized boxes. Since we're
// using the aspect ratio and rounding, if we're dealing with a 1px wide box,
// the height will be rounded to 1px most likely, which is within error
// margins. We want to throw out obviously too large and too small objects.
static const int MIN_DIAG = 30;
static const int MAX_DIAG = 150;

// This is a value used to speed up searching for boxes. If the distance
// between the first and last points of an object are greater than MAX_DIAG or
// less than MIN_HEIGHT (since they may be the width, height, or diagonal),
// we'll skip it.
static const int MIN_HEIGHT = 5;

// As a fail-safe when walking the edge of the potential boxes, we'll give up
// after moving this many pixels just in case something goes wrong (it's a
// while true loop). Boxes will never be four times the diagonal around, so
// this should be a safe value.
static const int MAX_ITERATIONS = MAX_DIAG*4;

// As a fail-safe, quit after this many iterations
static const int HIST_MAX = 10;

// What defines the huge jump in pixels
static const int HUGE_JUMP = 200;

// Total number of boxes, to verify we found the right ones. This is the number
// after we throw out the one or two above the huge jump to the first fill in
// section.
static const vsize_coord TOTAL_BOXES = 53;

// When determining the student ID, we need to know which boxes correspond to
// the ID. We also need to know how many digits the ID can be.
static const int ID_START  = 1;
static const int ID_END    = 10;
static const int ID_LENGTH = 10;

// What boxes are for the questions and how many
static const int Q_START   = 11;
static const int Q_END     = 44;
static const int Q_TOTAL   = 100;
static const int Q_OPTIONS = 5;

// What boxes are the bottom row below all the questions (used for finding
// filled bubbles)
static const int BOT_START = 45;
static const int BOT_END   = TOTAL_BOXES;

#endif
