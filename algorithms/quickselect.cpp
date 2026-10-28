#include "quickselect.hpp"

#include <vector>

int kthSmallest(std::vector<int>& nums, int left, int right, int k) {
    algorithms::quickselect(nums.data(), static_cast<std::size_t>(left),
                static_cast<std::size_t>(right), static_cast<std::size_t>(k),
                [](int x, int y) { return x < y; }, 16);
    return nums[k];
}
