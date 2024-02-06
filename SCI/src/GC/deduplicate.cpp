
#include "GC/deduplicate.h"
#include <cassert>
#include <cstdint>
using namespace std;

namespace sci {

  
inline void show(Integer *x, int size, string name) {
  cout << name << ": ";
  for (int i=0; i<size; i++) {
    cout << x[i].reveal<uint32_t>() << " ";
  }
  cout << endl;
}

Integer* deduplicate(Integer* in, int B, int N, int bitlength) {

  assert(is_power_of_2(N));
  auto logN = getLogOf(N);
  
  auto constantArray = getConstantArray(2*B, bitlength);
  auto zero = Bit(false);
  auto one = Bit(true);

  // Step 1. Oblivious sort
  sort(in, B);

  // Step 2. Append Dummies
  Integer* data = new Integer[2*B];
  memcpy(data, in, B*sizeof(Integer));
  for (int i = 0; i < B; i++) {
    data[B+i] = constantArray[i+1];
    data[B+i][logN] = one;
  }
  
  // Step 3. Assign tag bits
  // Bit* label = new Bit[2*B];
  // auto B_share = Integer(bitlength, B);
  // Integer counter(bitlength, 0);
  // Integer temp(bitlength, 0);
  // label[0] = one;
  // for (int i=1; i<2*B; i++) {
  //   auto cond = (counter < B_share);
  //   if (i < B) {
  //     cond = cond & (data[i] != data[i-1]);
  //   }
  //   label[i] = If(cond, one, zero);
  //   temp[0] = label[i];
  //   counter = counter + temp;
  // }
  Bit* label = new Bit[2*B];
  label[0] = one;
  for (int i=1; i<2*B; i++) {
    label[i] = If(data[i] != data[i-1], one, zero);
  }
  
  // Step 4. Oblivious Compaction
  ORCompact(label, std::vector{data}, 2*B, bitlength, constantArray); 

  delete[] constantArray;
  delete[] label;
  return data;
}

Integer* remap(Integer* in, Integer* inDeduplicated, Integer* resp, int b, int B, int N, int bitlength, int* cuckoo_map, Integer* constantArray) {
  
  auto zero = Bit(false);
  auto one = Bit(true);
  
  bool* cuckoo_label_plain = new bool[B]();
  for (int i=0; i<b; i++) {
    cuckoo_label_plain[cuckoo_map[i]] = 1;
  }
  auto cuckoo_label = new Bit[B];
  for (int j=0; j<B; j++) {
    cuckoo_label[j] = cuckoo_label_plain[j] ? one : zero;
  }

  ORCompact(cuckoo_label, std::vector{resp}, B, bitlength, constantArray);

  auto cuckoo_idx = new Integer[b];
  for (int i=0; i<b; i++) {
    cuckoo_idx[i] = constantArray[cuckoo_map[i]];
  }

  sort(cuckoo_idx, b, std::vector{resp});

  auto zero_integer = Integer(2, 0);
  auto one_integer = Integer(2, 1);

  // Step 1. Extend input
  auto idx = new Integer[2*b];
  auto extended_input = new Integer[2*b];
  auto extended_response = new Integer[2*b];
  auto tag = new Integer[2*b];
  auto label = new Bit[2*b];

  for (int i=0; i<b; i++) {
    idx[i] = constantArray[0];
    idx[i+b] = constantArray[i];
    extended_input[i] = inDeduplicated[i];
    extended_input[i+b] = in[i];
    extended_response[i] = resp[i];
    extended_response[i+b] = constantArray[0];
    tag[i] = zero_integer;
    tag[i+b] = one_integer;
  }
  for (int i=0; i<2*b; i++) {
    idx[i].resize(getLogOf(b)+1);
  }

  // Step 2. Sort by input
  sort(extended_input, 2*b, std::vector{idx, extended_response}, tag);
  for (int i=0; i<2*b; i++) {
    label[i] = tag[i][0];
  }

  // Step 3. Propagation
  for (int i=1; i<2*b; i++) {
    extended_response[i] = If(label[i], extended_response[i-1], extended_response[i]);
  }
    
  // Step 4. Oblivious Compaction
  ORCompact(label, std::vector{idx, extended_response}, 2*b, bitlength, constantArray);

  // Step 5. Oblivious sort w.r.t. idx
  sort(idx, b, std::vector{extended_response});

  delete [] idx;
  delete [] extended_input;
  delete [] tag;
  delete [] label;
  delete [] cuckoo_label_plain;
  delete [] cuckoo_label;
  delete [] cuckoo_idx;

  return extended_response;
}

} // namespace sci