#include <unordered_map>
#include <vector>

void edge_dict(const std::unordered_map<unsigned, std::vector<unsigned> > &d,
               std::unordered_map<std::pair<unsigned, unsigned>, unsigned> &l);
void list_objects_from_vlist(const std::unordered_map<unsigned, std::vector<unsigned> > &d,
                             const std::vector<unsigned> &vlist,
                             std::vector<std::vector<unsigned> > &a);
