
#include "GC/orcompact.h"
using namespace std;

namespace sci {

void OROffCompact(Bit* label, std::vector<Integer*> data, Integer* prefixSum, Integer& z, int n, int bitlen, Integer* constant) {
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

    std::vector<Integer*> dataPlusHalf = data;
    std::for_each(dataPlusHalf.begin(), dataPlusHalf.end(), [&](Integer*& ptr) {ptr += half;});

    OROffCompact(label, data, prefixSum, zModHalf, half, bitlen, constant);
    OROffCompact(label + half, dataPlusHalf, prefixSum + half, zPlusMModHalf, half, bitlen, constant);
    auto s = (zModHalfPlusM >= constant[half]) ^ (z >= constant[half]);
    // cout << fmt::format("s={}", s.reveal<bool>()) << endl;
    for (int i = 0; i < half; i++) {
      for (auto& datum: data) {
        swap(s^(constant[i] >= zPlusMModHalf), datum[i], datum[i+half]);
      }
    }
  }
}

void ORCompact(Bit* label, std::vector<Integer*> data, int n, int bitlen, Integer* constant) {
    // Compute prefix sum
    Integer* prefixSum = new Integer[n];
    prefixSum[0] = constant[0];
    Integer temp(bitlen, 0);
    for (int i = 1; i < n; i++) {
        temp[0] = label[i-1];
        prefixSum[i] = prefixSum[i-1] + temp;
    }
    OROffCompact(label, data, prefixSum, constant[0], n, bitlen, constant);
    delete[] prefixSum;
}

} // namespace sci