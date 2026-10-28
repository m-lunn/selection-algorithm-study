#include "floyd-rivest.hpp"

#include <vector>

int floydRivest(std::vector<int>& nums, int left, int right, int k) {
    algorithms::floyd_rivest(nums.data(), static_cast<std::size_t>(left),
                              static_cast<std::size_t>(right), static_cast<std::size_t>(k),
                              600, 16, [](int x, int y) { return x < y; });
    return nums[k];
}
