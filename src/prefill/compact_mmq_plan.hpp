#pragma once
#include <cstddef>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace strata::prefill::compact {
struct Group { size_t first, end, relative_bounds; int64_t row0, rows; };
// Keep the original gather groups, but split their products at expert boundaries
// whenever the bounded GU/H scratch would overflow. One expert is never split.
inline bool plan(const std::vector<int32_t>& absolute, size_t experts, size_t gather,
                 int64_t capacity, std::vector<int32_t>& bounds,
                 std::vector<int32_t>& local_ids, std::vector<Group>& groups) {
    if (absolute.size() != experts+1 || gather == 0 || capacity <= 0 || absolute[0] != 0) return false;
    for (size_t i=0; i<experts; ++i)
        if (absolute[i+1] < absolute[i] || absolute[i+1]-absolute[i] > capacity) return false;
    bounds=absolute; local_ids.resize((size_t)absolute.back()); groups.clear();
    for (size_t g=0; g<experts; g+=gather) {
        const size_t end=std::min(experts,g+gather);
        for (size_t first=g; first<end;) {
            size_t next=first+1;
            while (next<end && absolute[next+1]-absolute[first]<=capacity) ++next;
            const int64_t row0=absolute[first], rows=absolute[next]-row0;
            const size_t at=bounds.size();
            for (size_t i=first; i<=next; ++i) bounds.push_back((int32_t)(absolute[i]-row0));
            for (int64_t r=0; r<rows; ++r) local_ids[(size_t)(row0+r)]=(int32_t)r;
            groups.push_back({first,next,at,row0,rows}); first=next;
        }
    }
    return true;
}
} // namespace strata::prefill::compact
