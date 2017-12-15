#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <bsd/sys/queue.h>
#include <bsd/sys/tree.h>
#include <gmpxx.h>

#include <unordered_map>
#include <vector>
#include <iostream>

#define PRINT_GRAPH     0
#define PRINT_EDGE_LIST 0
#define PRINT_OBJECTS   0

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

void add_free_lists(struct object_list *olh, unsigned n)
{
    unsigned ecnt = 0;
    struct object *oi;
    uint8_t *edge_bit_map;
    TAILQ_FOREACH(oi, olh, entries)
    {
        for (unsigned eidx = 0; eidx < oi->elen; eidx++)
        {
            if (oi->edges[eidx] > ecnt)
            {
                ecnt = oi->edges[eidx];
            }
        }
    }

    /* Handle 0 indexing. */
    ecnt++;

    edge_bit_map = (uint8_t *)malloc(sizeof(*edge_bit_map)*ecnt);
    assert(edge_bit_map);
    memset(edge_bit_map, 0, sizeof(*edge_bit_map)*ecnt);

    unsigned idx = 0;
    TAILQ_FOREACH(oi, olh, entries)
    {
        for (unsigned j = 0; j < ecnt; j++)
        {
            struct object *oi2;
            int hit = 0;

            if (edge_bit_map[j]) continue;

            for (oi2 = TAILQ_NEXT(oi, entries);
                 oi2 != NULL;
                 oi2 = TAILQ_NEXT(oi2, entries))
            {
                for (unsigned eidx = 0;
                     eidx < oi2->elen;
                     eidx++)
                {
                    if (oi2->edges[eidx] == j)
                    {
                        hit = 1;
                        break;
                    }
                }

                if (hit) continue;
            }

            if (hit == 0)
            {
                struct int_s *is;
                is = (struct int_s *)malloc(sizeof(*is));
                assert(is);
                is->val = j;
                TAILQ_INSERT_TAIL(&oi->free_list, is, entries);
                edge_bit_map[j] = 1;
            }
        }

        idx++;
    }

    free(edge_bit_map);
}

TAILQ_HEAD(u8_fifo, u8_s);
struct u8_s
{
    uint8_t v;
    TAILQ_ENTRY(u8_s) entries;
};

void print_poly(std::unordered_map<uint64_t, mpz_class> &m)
{
    printf("{");
    for (auto const &pi : m)
    {
        //if (pi != m.begin()) printf(", ");
        printf("%lu: ", pi.first);
        std::cout << pi.second.get_str();
    }
    printf("}\n");
}

void enumerate(struct object_list *olh)
{
    struct u8_fifo indices_fifo =
        TAILQ_HEAD_INITIALIZER(indices_fifo);
    for (unsigned i = 0; i < 128; i++)
    {
        struct u8_s *e;
        e = (struct u8_s *)malloc(sizeof(*e));
        assert(e);
        e->v = i;
        TAILQ_INSERT_TAIL(&indices_fifo, e, entries);
    }

    std::unordered_map<uint16_t, uint16_t> idx_map;
    std::unordered_map<uint64_t, mpz_class> mpz_map0;

    mpz_class i1(1);
    mpz_map0.emplace(0, i1);

    struct object *oi;
    TAILQ_FOREACH(oi, olh, entries)
    {
        uint64_t exp2 = 0;

        for (unsigned eidx = 0; eidx < oi->elen; eidx++)
        {
            uint16_t val;
            uint16_t key = oi->edges[eidx];
            auto search = idx_map.find(key);
            if (search == idx_map.end())
            {
                struct u8_s *e = TAILQ_FIRST(&indices_fifo);
                assert(e);
                idx_map[key] = e->v;
                TAILQ_REMOVE(&indices_fifo, e, entries);
                free(e);
            }
            val = idx_map[key];
            assert(val < 64);
            exp2 |= 1ULL << val;
        }

        struct int_s *fi;
        uint64_t mask_free = 0;
        TAILQ_FOREACH(fi, &oi->free_list, entries)
        {
            struct u8_s *e;

            uint16_t key = fi->val;
            uint16_t val = idx_map[key];

            mask_free |= 1ULL << val;

            e = (u8_s *)malloc(sizeof(*e));
            assert(e);
            e->v = val;
            TAILQ_INSERT_HEAD(&indices_fifo, e, entries);
        }

        if (mask_free)
        {
            std::unordered_map<uint64_t, mpz_class> mpz_map1;
            mpz_map1.reserve(mpz_map0.size()*4);
            for (auto const &pi : mpz_map0)
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

                    auto search = mpz_map1.find(exp);
                    if (search != mpz_map1.end())
                    {
                        search->second = search->second + pi.second;
                    }
                    else
                    {
                        mpz_map1.emplace(exp, pi.second);
                    }
                }
            }

            mpz_map0.swap(mpz_map1);
        }
        else
        {
            std::vector<std::pair<uint64_t, mpz_class> > a;
            for (auto const &pi : mpz_map0)
            {
                uint64_t exp;
                uint64_t exp1 = pi.first;
                mpz_class v(pi.second);

                if (exp1 & exp2)
                {
                    continue;
                }

                exp = exp1 | exp2;

                auto search = mpz_map0.find(exp);
                if (search != mpz_map0.end())
                {
                    search->second = search->second + v;
                }
                else
                {
                    a.push_back(std::pair<uint64_t, mpz_class>(exp, v));
                }
            }

            for (auto const &ai : a)
            {
                mpz_map0[ai.first] = ai.second;
            }
        }
    }

    struct u8_s *e0 = TAILQ_FIRST(&indices_fifo);
    while (e0 != NULL)
    {
        struct u8_s *e1 = TAILQ_NEXT(e0, entries);
        free(e0);
        e0 = e1;
    }

    print_poly(mpz_map0);
}

int main(int argc, char *argv[])
{
    unsigned N;
    if (argc == 2)
    {
        N = strtoul(argv[1], NULL, 0);
    }
    else
    {
        printf("%s <N>\n", argv[0]);
        exit(0);
    }
    assert(N != 0);


    grid_graph(&vlh, N);
    edge_list(&elh, &vlh);
    object_list(&olh, &vlh, &elh);
    add_free_lists(&olh, N*N);

#if PRINT_GRAPH
    struct vertex *vi;
    TAILQ_FOREACH(vi, &vlh, entries)
    {
        printf("%d: ", vi->idx);
        for (unsigned vidx = 0; vidx < vi->nlen; vidx++)
        {
            printf("%d ", vi->neighbors[vidx]);
        }
        printf("\n");
    }
#endif

#if PRINT_EDGE_LIST
    struct edge *ei;
    TAILQ_FOREACH(ei, &elh, entries)
    {
        printf("%d: %d - %d\n", ei->idx, ei->n[0], ei->n[1]);
    }
#endif

#if PRINT_OBJECTS
    struct object *oi;
    TAILQ_FOREACH(oi, &olh, entries)
    {
        printf("(");
        for (unsigned oidx = 0; oidx < oi->elen; oidx++)
        {
            if (oidx != 0) printf(", ");
            printf("%d", oi->edges[oidx]);
        }
        printf(") free: ");

        struct int_s *si;
        TAILQ_FOREACH(si, &oi->free_list, entries)
        {
            printf("%d ", si->val); 
        }
        printf("\n");
    }
#endif

    enumerate(&olh);

}
