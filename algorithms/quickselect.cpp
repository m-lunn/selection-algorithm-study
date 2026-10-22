#include <vector>
#include <algorithm>

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

int kthSmallest(std::vector<int>& nums, int left, int right, int k) {
    if (left == right) {
        return nums[left]; // Only one element in the array
    }

    int pivotIndex = left + (right - left) / 2; // Choose a pivot index
    pivotIndex = partition(nums, left, right, pivotIndex);

    if (k == pivotIndex) {
        return nums[k]; // Found the k-th smallest element
    } else if (k < pivotIndex) {
        return kthSmallest(nums, left, pivotIndex - 1, k); // Search in the left part
    } else {
        return kthSmallest(nums, pivotIndex + 1, right, k); // Search in the right part
    }
}