
#include "GC/deduplicate.h"
#include <cassert>
using namespace std;

namespace sci {

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

Integer* remap(Integer* in, Integer* inDeduplicated, Integer* resp, int B, int N, int bitlength, Integer* constantArray) {
  auto zero = Integer(2, 0);
  auto one = Integer(2, 1);

  // Step 1. Extend input
  auto idx = new Integer[2*B];
  auto extended_input = new Integer[2*B];
  auto extended_response = new Integer[2*B];
  auto tag = new Integer[2*B];
  auto label = new Bit[2*B];

  for (int i=0; i<B; i++) {
    idx[i] = constantArray[0];
    idx[i+B] = constantArray[i];
    extended_input[i] = inDeduplicated[i];
    extended_input[i+B] = in[i];
    extended_response[i] = resp[i];
    extended_response[i+B] = constantArray[0];
    tag[i] = zero;
    tag[i+B] = one;
  }
  for (int i=0; i<2*B; i++) {
    idx[i].resize(getLogOf(B)+1);
  }

  // Step 2. Sort by input
  sort(extended_input, 2*B, std::vector{idx, extended_response}, tag);
  for (int i=0; i<2*B; i++) {
    label[i] = tag[i][0];
  }

  // Step 3. Propagation
  for (int i=1; i<2*B; i++) {
    extended_response[i] = If(label[i], extended_response[i-1], extended_response[i]);
  }
    
  // Step 4. Oblivious Compaction
  ORCompact(label, std::vector{idx, extended_response}, 2*B, bitlength, constantArray);

  // Step 5. Oblivious sort w.r.t. idx
  sort(idx, B, std::vector{extended_response});

  delete [] idx;
  delete [] extended_input;
  delete [] tag;
  delete [] label;

  return extended_response;
}

} // namespace sci