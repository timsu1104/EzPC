#include "GC/emp-sh2pc.h"
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <ratio>
#include <ctime>
#include <set>
#include <fmt/core.h>

using namespace sci;
using std::cout, std::endl;

int party, port = 8000, size = 512;
int bitlength = 32;
NetIO *io_gc;

void test_compaction() {
	Bit *label = new Bit[size];
	Integer *B = new Integer[size];

	int* in = new int[size];
	bool* labelvec = new bool[size];
	for (int i = 0, m = size >> 1; i < size; ++i) {
		if (rand() % (size - i) < m) {
			labelvec[i] = 1;
			m--;
		} else {
			labelvec[i] = 0;
		}
	}

// First specify Alice's input
	for(int i = 0; i < size; ++i) {
		label[i] = Bit(labelvec[i], ALICE);
	}

// Now specify Bob's input
	for(int i = 0; i < size; ++i) {
		in[i] = rand()%(1ULL << 20);
		B[i] = Integer(32, in[i], BOB);
	}
	
    auto comm_start = io_gc->counter;
	auto time_start = high_resolution_clock::now();
	
	auto constantArray = getConstantArray(size, bitlength);
	ORCompact(label, std::vector{B}, size, bitlength, constantArray);

	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
    cout << "elapsed " << time_span * 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;

	// cout << "in: ";
	// for(int i = 0; i < size; ++i)
	// 	cout << fmt::format("{} ", in[i]);
	// cout << endl;
	// cout << "label: ";
	// for(int i = 0; i < size; ++i)
	// 	cout << fmt::format("{} ", labelvec[i]);
	// cout << endl;
	// cout << "compacted: ";
	// for(int i = 0; i < size / 2; ++i)
	// 	cout << fmt::format("{} ", B[i].reveal<int32_t>());
	// cout << endl;

	// Verify
	std::multiset<int32_t> s;
	for(int i = 0; i < size / 2; ++i) {
		s.insert(B[i].reveal<int32_t>());
	}
	for(int i = 0; i < size; ++i) {
		if (labelvec[i]) {
			if (!s.count(in[i])) {
				error(fmt::format("{} does not exist!", in[i]).c_str());
			}
			s.extract(in[i]);
		}
	}
	cout << "Test passed" << endl;

	delete[] label;
	delete[] B;
	delete[] constantArray;
	delete[] in;
	delete[] labelvec;
}

int main(int argc, char **argv) {
	
  ArgMapping amap;
  amap.arg("r", party, "Role of party: ALICE = 1; BOB = 2");
  amap.arg("p", port, "Port Number");
  amap.arg("s", size, "number of total elements");
  amap.arg("l", bitlength, "bitlength of inputs");
  amap.parse(argc, argv);
  io_gc = new NetIO(party == ALICE ? nullptr : "127.0.0.1",
                    port + GC_PORT_OFFSET, true);

  setup_semi_honest(io_gc, party);
  test_compaction();
  io_gc->flush();
	cout << "# AND gates: " << circ_exec->num_and() << endl;
	if (party == ALICE)
		cout << "Setup time: " << circ_exec->total_time << "ms" << endl;
	else
	 	cout << "Online time: " << circ_exec->total_time << "ms" << endl;
}
