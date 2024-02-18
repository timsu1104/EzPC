#include "GC/emp-sh2pc.h"
#include <iostream>
#include <chrono>
#include <fmt/core.h>

using namespace sci;
using std::cout, std::endl;

int party, port = 8000, size = 256;
NetIO *io_gc;

void test_lowmc() {
	secret_block m;
	// m[0] = Integer(blocksize, 0xFFD5, BOB);
	for (int i=0; i<blocksize; i++) {
		m[i] = Integer(size, 0, BOB);
	}
	
	io_gc->start_record("construction");
	LowMC cipher(1, ALICE, size);
	io_gc->end_record("construction");

	io_gc->start_record("encrypt");
	auto res = cipher.encrypt(m);
	io_gc->end_record("encrypt");

	std::bitset<blocksize> ground_truth("1001101011011101110100110000000000000000110110010111100010000100");
	std::cout << endl;
	for (int i=0; i<blocksize; i++) {
		if (res[i][0].reveal() != ground_truth[i]) {
			error(fmt::format("{}-th position not align! ", i).c_str());
		}
	}
    cout << "Test Passed!" << endl;
}

int main(int argc, char **argv) {
	
	ArgMapping amap;
	amap.arg("r", party, "Role of party: ALICE = 1; BOB = 2");
	amap.arg("p", port, "Port Number");
	amap.arg("s", size, "number of total elements");
	amap.parse(argc, argv);

	io_gc = new NetIO(party == ALICE ? nullptr : "127.0.0.1",
						port + GC_PORT_OFFSET, true);

	auto time_start = high_resolution_clock::now();
	setup_semi_honest(io_gc, party);
	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
	cout << "General setup: elapsed " << time_span * 1000 << " ms." << endl;
	test_lowmc();
	cout << "# AND gates: " << circ_exec->num_and() << endl;
}
