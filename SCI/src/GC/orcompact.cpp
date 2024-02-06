
#include "GC/orcompact.h"
#include <cstddef>
using namespace std;

namespace sci {

std::vector<Integer*> offset(std::vector<Integer*> data, int offset) {
  std::vector<Integer*> dataPlusHalf = data;
  std::for_each(dataPlusHalf.begin(), dataPlusHalf.end(), [&](Integer*& ptr) {ptr += offset;});
  return dataPlusHalf;
}

void OROffCompact(Bit* label, std::vector<Integer*> data, Integer* prefixSum, Integer& z, int n, int bitlen, Integer* constant) {
  assert(is_power_of_2(n));
  if (n == 2) {
    for (auto& datum: data) {
      swap(((!label[0]) & label[1]) ^ z[0], datum[0], datum[1]);
    }
  } else if (n > 2) {
    int half = n >> 1;

    // Some intermediate variables
    auto m = prefixSum[half] - prefixSum[0];
    auto logHalf = getLogOf(half);
    auto zModHalf = mod(z, logHalf, constant[0][0]); // z mod n/2
    auto zModHalfPlusM = zModHalf + m; // (z mod n/2) + m
    auto zPlusMModHalf = mod(zModHalfPlusM, logHalf, constant[0][0]); // (z + m) mod n/2
    
    // cout << fmt::format("z={}, n={}", z.reveal<int32_t>(), n) << endl;
    // cout << fmt::format("half={}, logHalf={}, m={}", half, logHalf, m.reveal<int32_t>()) << endl;
    // cout << fmt::format("zModHalf={}", zModHalf.reveal<int32_t>()) << endl;
    // cout << fmt::format("zModHalfPlusM={}", zModHalfPlusM.reveal<int32_t>()) << endl;
    // for (int i=0; i < zModHalfPlusM.bits.size()-1; i++) {
    //     cout << fmt::format("{} ", zModHalfPlusM[i].reveal<bool>());
    // }
    // cout << fmt::format("zPlusMModHalf={}", zPlusMModHalf.reveal<int32_t>()) << endl;

    OROffCompact(label, data, prefixSum, zModHalf, half, bitlen, constant);
    OROffCompact(label + half, offset(data, half), prefixSum + half, zPlusMModHalf, half, bitlen, constant);
    auto s = (zModHalfPlusM >= constant[half]) ^ (z >= constant[half]);
    // cout << fmt::format("s={}", s.reveal<bool>()) << endl;
    for (int i = 0; i < half; i++) {
      auto sel = s^(constant[i] >= zPlusMModHalf);
      for (auto& datum: data) {
        swap(sel, datum[i], datum[i+half]);
      }
    }
  }
}

void ORCompact(Bit* label, std::vector<Integer*> data, int n, int bitlen, Integer* constant, Integer* prefixSum) {
  if (n < 2) return;
  // Compute prefix sum
  bool init_flag = false;
  if (prefixSum == nullptr) {
    init_flag = true;
    prefixSum = new Integer[n];
    prefixSum[0] = constant[0];
    Integer temp(bitlen, 0);
    for (int i = 1; i < n; i++) {
        temp[0] = label[i-1];
        prefixSum[i] = prefixSum[i-1] + temp;
    }
  }
  int n1 = greatestPowerOfTwoLessThan(n);

  if (n == n1) {
    OROffCompact(label, data, prefixSum, constant[0], n1, bitlen, constant);
  } else {
    int n2 = n - n1;
    auto m = prefixSum[n2] - prefixSum[0];
    auto z = mod(constant[n2] + m, getLogOf(n1), constant[0][0]);

    ORCompact(label, data, n2, bitlen, constant, prefixSum);
    OROffCompact(label + n2, offset(data, n2), prefixSum + n2, z, n1, bitlen, constant);
    
    for (int i = 0; i < n2; i++) {
      auto sel = constant[i] >= m;
      for (auto& datum: data) {
        swap(sel, datum[i], datum[i + n1]);
      }
    }
  }
  if (init_flag) 
    delete [] prefixSum;
}

} // namespace sci