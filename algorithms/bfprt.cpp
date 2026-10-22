#include <algorithm>
#include <stdexcept>
#include <vector>

int selectByMedianOfMedians(std::vector<int>& nums, int left, int right, int k, int groupSize) {
    while (left < right) {
        int length = right - left + 1;

        if (length <= groupSize) {
            std::sort(nums.begin() + left, nums.begin() + right + 1);
            return nums[k];
        }

        int groupCount = (length + groupSize - 1) / groupSize;
        for (int group = 0; group < groupCount; group++) {
            int groupLeft = left + group * groupSize;
            int groupRight = std::min(groupLeft + groupSize - 1, right);
            int groupMiddle = groupLeft + (groupRight - groupLeft) / 2;

            std::sort(nums.begin() + groupLeft, nums.begin() + groupRight + 1);
            std::swap(nums[left + group], nums[groupMiddle]);
        }

        int mediansLeft = left;
        int mediansRight = left + groupCount - 1;
        int mediansK = mediansLeft + (groupCount - 1) / 2;
        int pivotValue = selectByMedianOfMedians(
            nums,
            mediansLeft,
            mediansRight,
            mediansK,
            groupSize
        );

        int less = left;
        int current = left;
        int greater = right;

        while (current <= greater) {
            if (nums[current] < pivotValue) {
                std::swap(nums[less++], nums[current++]);
            } else if (nums[current] > pivotValue) {
                std::swap(nums[current], nums[greater--]);
            } else {
                current++;
            }
        }

        if (k < less) {
            right = less - 1;
        } else if (k > greater) {
            left = greater + 1;
        } else {
            return nums[k];
        }
    }

    return nums[left];
}

int bfprt(std::vector<int>& nums, int left, int right, int k, int groupSize = 5) {
    if (groupSize < 3 || groupSize % 2 == 0) {
        throw std::invalid_argument("groupSize must be an odd number greater than or equal to 3");
    }
    if (left < 0 || right >= static_cast<int>(nums.size()) || left > right) {
        throw std::out_of_range("left and right must describe a non-empty range in nums");
    }
    if (k < left || k > right) {
        throw std::out_of_range("k must be an index within the selected range");
    }

    return selectByMedianOfMedians(nums, left, right, k, groupSize);
}
