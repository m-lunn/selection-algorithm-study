#include "introselect.hpp"

#include <vector>

int introselect(std::vector<int>& nums, int left, int right, int k) {
    algorithms::introselect(nums.data() + left, static_cast<std::size_t>(right - left + 1),
                             static_cast<std::size_t>(k - left), 16, 2.0, 5,
                             [](int x, int y) { return x < y; });
    return nums[k];
}
