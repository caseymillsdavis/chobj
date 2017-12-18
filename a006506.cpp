#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <iostream>

#include <unordered_map>
#include <vector>
#include <stack>

#include <gmpxx.h>

typedef std::vector<unsigned> hobj_t;
typedef std::vector<unsigned> free_list_t;
typedef std::vector<std::pair<hobj_t, free_list_t> > hobj_list_t;
typedef std::unordered_map<uint64_t, mpz_class> poly_t;

static void grid_graph_objects(hobj_list_t &hl, unsigned n)
{
    for (unsigned i = 0; i < n; i++)
    {
        for (unsigned j = 0; j < n; j++)
        {
            hobj_t ho;
            free_list_t fl;

            /* Left */
            if (j != 0)
            {
                unsigned e = i*(n-1) + j-1;
                ho.push_back(e);
                fl.push_back(e);
            }

            /* Right */
            if (j != n-1)
            {
                ho.push_back(i*(n-1) + j);
            }

            /* Up */
            if (i != 0)
            {
                unsigned e = n*(n-1) + (i-1)*n + j;
                ho.push_back(e);
                fl.push_back(e);
            }

            /* Down */
            if (i != n-1)
            {
                ho.push_back(n*(n-1) + i*n + j);
            }

            hl.push_back(std::pair<hobj_t, free_list_t>(ho, fl));
        }
    }
}

static void print_hobj_list(const hobj_list_t &hl)
{
    for (auto const &h : hl)
    {
        const hobj_t &ho = h.first;
        const free_list_t &fl = h.second;

        std::cout << "([";
        for (auto const &e : ho)
        {
            std::cout << e << " ";
        }
        std::cout << "], [";

        for (auto const &e : fl)
        {
            std::cout << e << " ";
        }
        std::cout << "])" << std::endl;
    }
}

static void print_poly(poly_t &p)
{
    std::cout << "{";
    for (auto const &pi : p)
    {
        std::cout << pi.first << ": " << pi.second.get_str() << " ";
    }
    std::cout << "}" << std::endl;
}

static void enumerate(const hobj_list_t &hl)
{
    poly_t poly0;
    std::unordered_map<unsigned, uint8_t> idx_map;
    std::stack<uint8_t> indices_fifo;

    for (unsigned i = 0; i < 64; i++)
    {
        indices_fifo.push(i);
    }

    mpz_class i1(1);
    poly0.emplace(0, i1);

    for (auto const &h : hl)
    {
        const hobj_t &ho = h.first;
        const free_list_t &fl = h.second;

        uint64_t exp2 = 0;
        uint64_t mask_free = 0;

        for (auto const &e : ho)
        {
            if (idx_map.find(e) == idx_map.end())
            {
                assert(!indices_fifo.empty());
                idx_map[e] = indices_fifo.top();
                indices_fifo.pop();
            }
            exp2 |= 1ULL << idx_map[e];
        }

        for (auto const &e : fl)
        {
            uint8_t val = idx_map[e];
            mask_free |= 1ULL << val;
            indices_fifo.push(val);
        }

        {
            poly_t poly1;
            poly1.rehash(poly0.bucket_count());
            poly1.reserve(poly0.size()*4);
            for (auto const &pi : poly0)
            {
                uint64_t exp1 = pi.first;

                for (unsigned ii = 0; ii < 2; ii++)
                {
                    uint64_t exp;

                    if (ii == 0)
                    {
                        exp = exp1;
                    }
                    else
                    {
                        if (exp1 & exp2)
                        {
                            continue;
                        }
                        exp = exp1 | exp2;
                    }

                    exp ^= (exp & mask_free);

                    auto search = poly1.find(exp);
                    if (search != poly1.end())
                    {
                        search->second = search->second + pi.second;
                    }
                    else
                    {
                        poly1.emplace(exp, pi.second);
                    }
                }
            }

            poly0.swap(poly1);
        }
    }

    print_poly(poly0);
}

int main(int argc, char *argv[])
{
    unsigned N;
    hobj_list_t hl;

    if (argc == 2)
    {
        N = strtoul(argv[1], NULL, 0);
    }

    if (argc != 2 || N < 2)
    {
        std::cout << argv[0] << " <N>" << std::endl;
        exit(0);
    }

    grid_graph_objects(hl, N);
    enumerate(hl);
}
