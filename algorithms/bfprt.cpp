#include "bfprt.hpp"

#include <vector>

int bfprt(std::vector<int>& nums, int left, int right, int k, int groupSize) {
    algorithms::bfprt(nums.data(), static_cast<std::size_t>(left),
                      static_cast<std::size_t>(right), static_cast<std::size_t>(k),
                      static_cast<std::size_t>(groupSize), 16,
                      [](int x, int y) { return x < y; });
    return nums[k];
}
