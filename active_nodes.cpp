#include "active_nodes.h"

namespace std
{
    template <>
    class hash<pair<unsigned, unsigned> >
    {
    public:
        size_t operator()(const pair<unsigned, unsigned> &v) const
        {
            return hash<unsigned>()(v.first) ^ hash<unsigned>()(v.second);
        }
    };
}

void edge_dict(const std::unordered_map<unsigned, std::vector<unsigned> > &d,
               std::unordered_map<std::pair<unsigned, unsigned>, unsigned> &dn)
{
    unsigned c = 0;
    for (auto const &di : d)
    {
        for (auto const &ni : di.second)
        {
            unsigned v0, v1;
            v0 = di.first;
            v1 = ni;
            if (v1 < v0) std::swap(v0, v1);
            std::pair<unsigned, unsigned> a(v0, v1);
            auto search = dn.find(a);
            if (search != dn.end()) continue;
            dn[a] = c;
            c++;
        }
    }
}

void list_objects_from_vlist(const std::unordered_map<unsigned, std::vector<unsigned> > &d,
                             const std::vector<unsigned> &vlist,
                             std::vector<std::vector<unsigned> > &a)
{
    std::unordered_map<std::pair<unsigned, unsigned>, unsigned> dn;
    edge_dict(d, dn);
    for (auto const &k : vlist)
    {
        std::vector<unsigned> w;
        const std::vector<unsigned> &b = d.at(k);
        for (auto const &k2 : b)
        {
            unsigned v0, v1;
            v0 = k;
            v1 = k2;
            if (v1 < v0) std::swap(v0, v1);
            std::pair<unsigned, unsigned> t(v0, v1);
            unsigned n = dn[t];
            w.emplace_back(n);
        }
        a.emplace_back(w);
    }
}
