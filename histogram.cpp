#include <algorithm>

#include "math.h"
#include "options.h"
#include "histogram.h"

Histogram::Histogram(const std::vector<std::vector<unsigned char>>& img)
    : graph(256, 0) // This is unsigned char, so there's 0-255
{
    int h = img.size();
    int w = (h>0)?img[0].size():0;
    total = w*h;

    // Generate the graph by counting how many pixels are each shade. This
    // is easy with discrete values, would be more interesting with doubles.
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            ++graph[img[y][x]];
}

// This simple algorithm worked just as good and executed faster than some
// others I found online. Thus, I'll keep this simple one.
unsigned char Histogram::threshold(unsigned char initial) const
{
    typedef std::vector<int>::const_iterator iterator;

    // This is my algorithm that currently works better than the one above.
    const iterator half = graph.begin() + initial;
    iterator black_max = std::max_element(graph.begin(), half);
    iterator white_max = std::max_element(half, graph.end());

    if (black_max != half && white_max != graph.end())
        return 0.5*((black_max-graph.begin()) + (white_max-graph.begin()));

    return initial;
}
