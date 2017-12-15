#include "graphs_gen.h"

void grid_graph(std::unordered_map<unsigned, std::vector<unsigned> > &d, unsigned n)
{
    for (unsigned i = 0; i < n*n; i++)
    {
        std::vector<unsigned> neighbs;

        /* neighbors up, left, right, down */
        if (i > n-1)     neighbs.push_back(i-n);
        if (i%n != 0)    neighbs.push_back(i-1);
        if (i%n != n-1)  neighbs.push_back(i+1);
        if (i < n*(n-1)) neighbs.push_back(i+n);

        d[i] = neighbs;
    }
}
