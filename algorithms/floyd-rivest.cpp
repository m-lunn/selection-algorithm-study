#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

const int kSampleThreshold = 600;  // the original paper's cutoff

void selectFloydRivest(std::vector<int>& nums, int left, int right, int k) {
    while (right > left) {
        // Step 1: on a big range, recursively select k inside a small window.
        if (right - left > kSampleThreshold) {
            double n = right - left + 1;
            double i = k - left + 1;
            double z = std::log(n);
            double s = 0.5 * std::exp(2.0 * z / 3.0); // window size
            double sd = 0.5 * std::sqrt(z * s * (n - s) / n); // safety margin
            if (i < n / 2) sd = -sd; // shift toward the near end

            int newLeft = std::max(left, static_cast<int>(k - i * s / n + sd));
            int newRight = std::min(right, static_cast<int>(k + (n - i) * s / n + sd));
            selectFloydRivest(nums, newLeft, newRight, k);
        }

        // Step 2: partition nums[left..right] around t = nums[k].
        int t = nums[k];
        int a = left;
        int b = right;

        // Put t at the left end and something >= t at the right end, so both
        // scans below are guaranteed to stop inside the range.
        std::swap(nums[left], nums[k]);
        if (nums[right] > t) std::swap(nums[right], nums[left]);

        while (a < b) {
            std::swap(nums[a], nums[b]);
            a++;
            b--;
            while (nums[a] < t) a++;
            while (nums[b] > t) b--;
        }

        // Move the pivot into its final position b.
        if (nums[left] == t) {
            std::swap(nums[left], nums[b]);
        } else {
            b++;
            std::swap(nums[b], nums[right]);
        }

        // Step 3: keep only the side that contains k.
        if (b <= k) left = b + 1;
        if (k <= b) right = b - 1;
    }
}

}

int floydRivest(std::vector<int>& nums, int left, int right, int k) {
    if (left < 0 || right >= static_cast<int>(nums.size()) || left > right) {
        throw std::out_of_range("left and right must describe a non-empty range in nums");
    }
    if (k < left || k > right) {
        throw std::out_of_range("k must be an index within the selected range");
    }

    selectFloydRivest(nums, left, right, k);
    return nums[k];
}