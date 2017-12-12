#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <bsd/sys/queue.h>
#include <bsd/sys/tree.h>
#include <gmp.h>

#define PRINT_GRAPH     0
#define PRINT_EDGE_LIST 0
#define PRINT_OBJECTS   0

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

TAILQ_HEAD(vertex_list, vertex);
struct vertex
{
    int idx;
    int *neighbors;
    unsigned nlen;
    TAILQ_ENTRY(vertex) entries;
};

TAILQ_HEAD(edge_list, edge);
struct edge
{
    int idx;
    int n[2];
    TAILQ_ENTRY(edge) entries;
};

TAILQ_HEAD(int_list, int_s);
struct int_s
{
    int16_t val;
    TAILQ_ENTRY(int_s) entries;
};

TAILQ_HEAD(object_list, object);
struct object
{
    struct int_list free_list;
    int *edges;
    unsigned elen;
    TAILQ_ENTRY(object) entries;
};

void grid_graph(struct vertex_list *vlh, unsigned n)
{
    for (unsigned i = 0; i < n*n; i++)
    {
        struct vertex *v;
        int vidx = 0;
        int va[4];

        /* neighbors up, left, right, down */
        if (i > n-1) va[vidx++] = i - n;
        if (i%n != 0) va[vidx++] = i-1;
        if (i%n != n-1) va[vidx++] = i+1;
        if (i < n*(n-1)) va[vidx++] = i + n;

        v = malloc(sizeof(*v)); 
        assert(v);
        v->neighbors = malloc(sizeof(int)*vidx);
        assert(v->neighbors);
        v->idx = i;
        v->nlen = vidx;
        memcpy(v->neighbors, va, vidx*sizeof(int));
        TAILQ_INSERT_TAIL(vlh, v, entries);
    }
}

void edge_list(struct edge_list *elh,
               struct vertex_list *vlh)
{
    unsigned eidx = 0;
    struct vertex *vi;
    TAILQ_FOREACH(vi, vlh, entries)
    {
        int v0 = vi->idx;
        for (unsigned i = 0; i < vi->nlen; i++)
        {
            int v1 = vi->neighbors[i];
            int found = 0;
            int n[2];
            struct edge *ei, *e;

            if (v1 < v0)
            {
                n[0] = v1;
                n[1] = v0;
            }
            else
            {
                n[0] = v0;
                n[1] = v1;
            }

            TAILQ_FOREACH(ei, elh, entries)
            {
                if (ei->n[0] == n[0] &&
                    ei->n[1] == n[1])
                {
                    found = 1;
                    break;
                }
            }

            if (found) continue;

            e = malloc(sizeof(*e));
            assert(e);
            e->idx = eidx++;
            e->n[0] = n[0];
            e->n[1] = n[1];
            TAILQ_INSERT_TAIL(elh, e, entries);
        }
    }
}

void object_list(struct object_list *olh,
                 struct vertex_list *vlh,
                 struct edge_list *elh)
{
    struct vertex *vi;
    TAILQ_FOREACH(vi, vlh, entries)
    {
        struct object *o;
        int *edges;

        edges = malloc(sizeof(int)*vi->nlen);
        assert(edges);

        for (unsigned i = 0; i < vi->nlen; i++)
        {
            struct edge *ei;
            int n[2];
            int v0 = vi->idx;
            int v1 = vi->neighbors[i];
            int pushed = 0;

            if (v1 < v0)
            {
                n[0] = v1;
                n[1] = v0;
            }
            else
            {
                n[0] = v0;
                n[1] = v1;
            }

            TAILQ_FOREACH(ei, elh, entries)
            {
                if (ei->n[0] == n[0] &&
                    ei->n[1] == n[1])
                {
                    edges[i] = ei->idx;
                    pushed = 1;
                    break;
                }
            }

            assert(pushed);
        }

        o = malloc(sizeof(*o));
        assert(o);
        TAILQ_INIT(&o->free_list);
        o->edges = edges;
        o->elen = vi->nlen;
        TAILQ_INSERT_TAIL(olh, o, entries);
    }
}

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

    edge_bit_map = malloc(sizeof(*edge_bit_map)*ecnt);
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
                is = malloc(sizeof(*is));
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

RB_HEAD(u16_rb, u16_s);
struct u16_s
{
    uint16_t key;
    uint16_t val;
    RB_ENTRY(u16_s) entries;
};
//RB_PROTOTYPE(u16_rb, u16_s, entries, u16_rb_cmp); 
int u16_rb_cmp(struct u16_s *a, struct u16_s *b)
{
    return (int)a->key - (int)b->key;
}
RB_GENERATE(u16_rb, u16_s, entries, u16_rb_cmp); 

RB_HEAD(u32_rb, mpz_rb_e);
struct mpz_rb_e
{
    uint64_t key;
    mpz_t val;
    RB_ENTRY(mpz_rb_e) entries;
};
int u32_rb_cmp(struct mpz_rb_e *a, struct mpz_rb_e *b)
{
    if (a->key < b->key)
    {
        return -1;
    }
    else if (a->key > b->key)
    {
        return 1;
    }

    return 0;
}
RB_GENERATE(u32_rb, mpz_rb_e, entries, u32_rb_cmp); 

TAILQ_HEAD(u32_list, u32l_s);
struct u32l_s
{
    uint64_t key;
    mpz_t val;
    TAILQ_ENTRY(u32l_s) entries;
};

void print_poly(struct u32_rb *p)
{
    struct mpz_rb_e *pi;
    printf("{");
    RB_FOREACH(pi, u32_rb, p)
    {
        if (pi != RB_MIN(u32_rb, p)) printf(", ");
        printf("%lu: ", pi->key);
        mpz_out_str(stdout, 10, pi->val);
    }
    printf("}\n");
}

void enumerate(struct object_list *olh)
{
    struct u8_fifo indices_fifo =
        TAILQ_HEAD_INITIALIZER(indices_fifo);
    for (unsigned i = 0; i < 32; i++)
    {
        struct u8_s *e;
        e = malloc(sizeof(*e));
        assert(e);
        e->v = i;
        TAILQ_INSERT_TAIL(&indices_fifo, e, entries);
    }

    struct u16_rb idx_map = RB_INITIALIZER(idx_map);
    struct u32_rb poly0 = RB_INITIALIZER(poly0);
    struct u32_rb *poly0_ptr = &poly0;
    struct u32_rb poly1 = RB_INITIALIZER(poly1);
    struct u32_rb *poly1_ptr = &poly1;

    struct mpz_rb_e *p0;
    p0 = malloc(sizeof(*p0));
    assert(p0);
    p0->key = 0;
    mpz_init_set_ui(p0->val, 1);

    RB_INSERT(u32_rb, poly0_ptr, p0);

    struct object *oi;
    TAILQ_FOREACH(oi, olh, entries)
    {
        uint64_t exp2 = 0;

        for (unsigned eidx = 0; eidx < oi->elen; eidx++)
        {
            struct u16_s find, *j, *k;
            find.key = oi->edges[eidx];
            j = RB_FIND(u16_rb, &idx_map, &find); 
            if (j == NULL)
            {
                struct u8_s *e = TAILQ_FIRST(&indices_fifo);
                assert(e);
                j = malloc(sizeof(*j));
                j->key = find.key;
                j->val = e->v;
                k = RB_INSERT(u16_rb, &idx_map, j);
                assert(k == NULL);
                TAILQ_REMOVE(&indices_fifo, e, entries);
                free(e);
            }
            assert(j->val < 32);
            exp2 |= 1 << j->val;
        }

        struct int_s *fi;
        uint64_t mask_free = 0;
        TAILQ_FOREACH(fi, &oi->free_list, entries)
        {
            struct u16_s find, *j;
            struct u8_s *e;

            find.key = fi->val;
            j = RB_FIND(u16_rb, &idx_map, &find); 
            assert(j);
            fi->val = j->val;
            mask_free |= 1 << fi->val;

            e = malloc(sizeof(*e));
            assert(e);
            e->v = fi->val;
            TAILQ_INSERT_HEAD(&indices_fifo, e, entries);
        }

        if (mask_free)
        {
            struct mpz_rb_e *pi;
            RB_FOREACH(pi, u32_rb, poly0_ptr)
            {
                /* TODO(CMD): make v a big num. */
                uint64_t exp;
                uint64_t exp1 = pi->key;
                mpz_t v, v1;
                mpz_init(v);
                mpz_init_set(v1, pi->val);

                for (unsigned ii = 0; ii < 2; ii++)
                {
                    if (ii == 0)
                    {
                        exp = exp1;
                        mpz_set(v, v1);
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

                    struct mpz_rb_e *crb, find;
                    find.key = exp;
                    crb = RB_FIND(u32_rb, poly1_ptr, &find); 
                    if (crb != NULL)
                    {
                        mpz_add(crb->val, crb->val, v);
                    }
                    else
                    {
                        struct mpz_rb_e *p1;
                        p1 = malloc(sizeof(*p1));
                        assert(p1);
                        p1->key = exp;
                        mpz_init_set(p1->val, v);
                        RB_INSERT(u32_rb, poly1_ptr, p1);
                    }
                }
            }

            /* Swap poly0 and poly1. */
            struct mpz_rb_e *d0, *d1;
            struct u32_rb *temp;
            for (d0 = RB_MIN(u32_rb, poly0_ptr);
                 d0 != NULL;
                 d0 = d1)
            {
                d1 = RB_NEXT(u32_rb, poly0_ptr, d0);
                RB_REMOVE(u32_rb, poly0_ptr, d0);
                free(d0);
            }
            temp = poly0_ptr;
            poly0_ptr = poly1_ptr;
            poly1_ptr = temp;
        }
        else
        {
            struct u32_list a = TAILQ_HEAD_INITIALIZER(a);
            struct mpz_rb_e *pi;
            RB_FOREACH(pi, u32_rb, poly0_ptr)
            {
                uint64_t exp;
                uint64_t exp1 = pi->key;
                mpz_t v;
                mpz_init_set(v, pi->val);

                if (exp1 & exp2)
                {
                    continue;
                }

                exp = exp1 | exp2;

                struct mpz_rb_e *crb, find;
                find.key = exp;
                crb = RB_FIND(u32_rb, poly0_ptr, &find); 
                if (crb != NULL)
                {
                    mpz_add(crb->val, crb->val, v);
                }
                else
                {
                    struct u32l_s *av;
                    av = malloc(sizeof(*av));
                    assert(av);
                    av->key = exp;
                    mpz_init_set(av->val, v);
                    TAILQ_INSERT_HEAD(&a, av, entries);
                }
            }

            /* Insert a-list into polynomial */
            struct u32l_s *av0, *av1;
            for (av0 = TAILQ_FIRST(&a);
                 av0 != NULL;
                 av0 = av1)
            {
                av1 = TAILQ_NEXT(av0, entries);

                struct mpz_rb_e *p1;
                p1 = malloc(sizeof(*p1));
                assert(p1);
                p1->key = av0->key;
                mpz_init_set(p1->val, av0->val);

                free(av0);

                RB_INSERT(u32_rb, poly0_ptr, p1);
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

    print_poly(poly0_ptr);
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

    struct vertex_list vlh = TAILQ_HEAD_INITIALIZER(vlh);
    struct edge_list elh = TAILQ_HEAD_INITIALIZER(elh);
    struct object_list olh = TAILQ_HEAD_INITIALIZER(olh);

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

    /* Free object list. */
    struct object *o1 = TAILQ_FIRST(&olh);
    while (o1 != NULL)
    {
        struct object *o2 = TAILQ_NEXT(o1, entries);

        /* Free object-free lists. */
        struct int_s *is1 = TAILQ_FIRST(&o1->free_list);
        while (is1 != NULL)
        {
            struct int_s *is2 = TAILQ_NEXT(is1, entries);
            free(is1);
            is1 = is2;
        }
        TAILQ_INIT(&o1->free_list);

        free(o1->edges);
        free(o1);
        o1 = o2;
    }
    TAILQ_INIT(&olh);

    /* Free edge list. */
    struct edge *e1 = TAILQ_FIRST(&elh);
    while (e1 != NULL)
    {
        struct edge *e2 = TAILQ_NEXT(e1, entries);
        free(e1);
        e1 = e2;
    }
    TAILQ_INIT(&elh);

    /* Free vertex list. */
    struct vertex *v1 = TAILQ_FIRST(&vlh);
    while (v1 != NULL)
    {
        struct vertex *v2 = TAILQ_NEXT(v1, entries);
        free(v1->neighbors);
        free(v1);
        v1 = v2;
    }
    TAILQ_INIT(&vlh);
}
