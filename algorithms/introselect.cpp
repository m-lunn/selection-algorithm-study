#include <algorithm>
#include <vector>

int partition(std::vector<int>& nums, int left, int right, int pivotIndex) {
    int pivotValue = nums[pivotIndex];
    std::swap(nums[pivotIndex], nums[right]); // Move pivot to end
    int storeIndex = left;

    for (int i = left; i < right; i++) {
        if (nums[i] < pivotValue) {
            std::swap(nums[storeIndex], nums[i]);
            storeIndex++;
        }
    }
    std::swap(nums[right], nums[storeIndex]); // Move pivot to its final place
    return storeIndex;
}

int heapsortSelection(std::vector<int>& nums, int left, int right, int k) {
    std::make_heap(nums.begin() + left, nums.begin() + right + 1);
    std::sort_heap(nums.begin() + left, nums.begin() + right + 1);
    return nums[k];
}

int introselect(std::vector<int>& nums, int left, int right, int k) {
    int depthLimit = 0;
    for (int size = right - left + 1; size > 1; size >>= 1) {
        depthLimit += 2;
    }

    while (left < right) {
        if (depthLimit == 0) {
            return heapsortSelection(nums, left, right, k);
        }
        depthLimit--;

        int pivotIndex = left + (right - left) / 2;
        pivotIndex = partition(nums, left, right, pivotIndex);

        if (k == pivotIndex) {
            return nums[k];
        } else if (k < pivotIndex) {
            right = pivotIndex - 1;
        } else {
            left = pivotIndex + 1;
        }
    }

    return nums[left];
}