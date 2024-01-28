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

int party, port = 8000, size = 256;
int bitlength = 32;
NetIO *io_gc;

void test_deduplication() {
	Integer *B = new Integer[size];

	int* in = new int[size];
	
// Now specify Alice's input

// Now specify Bob's input
	for(int i = 0; i < size; ++i) {
		in[i] = rand()%(size >> 1);
		B[i] = Integer(bitlength, in[i], BOB);
	}
	
    auto comm_start = io_gc->counter;
	auto time_start = high_resolution_clock::now();
	
	auto res = deduplicate(B, size, (1ULL << 16), bitlength);

	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
    cout << "elapsed " << time_span * 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;

	// cout << "in: ";
	// for(int i = 0; i < size; ++i)
	// 	cout << fmt::format("{} ", in[i]);
	// cout << endl;
	// cout << "deduplicated: ";
	// for(int i = 0; i < size; ++i)
	// 	cout << fmt::format("{} ", res[i].reveal<int32_t>());
	// cout << endl;

	// Verify
	std::set<int32_t> s;
	for(int i = 0; i < size; ++i) {
		auto result = res[i].reveal<int32_t>();
		if (s.count(result)) {
			error(fmt::format("{} repeated!", result).c_str());
		}
		s.insert(result);
	}
	for(int i = 0; i < size; ++i) {
		if (!s.count(in[i])) {
			error(fmt::format("{} does not exist!", in[i]).c_str());
		}
	}
	cout << "Test passed" << endl;

	delete[] in;
	delete[] res;
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

	auto time_start = high_resolution_clock::now();
	setup_semi_honest(io_gc, party);
	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
	cout << "General setup: elapsed " << time_span * 1000 << " ms." << endl;
	test_deduplication();
	io_gc->flush();
}
