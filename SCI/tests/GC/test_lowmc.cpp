#include "GC/emp-sh2pc.h"
#include <iostream>
#include <chrono>
#include <fmt/core.h>
#include <vector>

using namespace sci;
using std::cout, std::endl;

int party, port = 8000, size = 256;
NetIO *io_gc;

void test_lowmc() {
	std::vector<secret_block> m(size, Integer(blocksize, rand(), BOB));
	m[0] = Integer(blocksize, 0xFFD5, BOB);
	
	auto time_start = high_resolution_clock::now();

	LowMC cipher(1, ALICE);
	
    cout << "Built cipher: elapsed " << time_from(time_start) / 1000.0 << " ms." << endl;

    auto comm_start = io_gc->counter;
	time_start = high_resolution_clock::now();

	auto res = cipher.encrypt(m);

    cout << "Encryption: elapsed " << time_from(time_start) / 1000.0 << " ms." << endl;
    cout << "Encryption: sent " << (io_gc->counter - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;

	std::bitset<blocksize> ground_truth("0111010001011000111001110000111110101110010110001100011101011001");
	for (int i=0; i<blocksize; i++) {
		if (res[0][i].reveal() != ground_truth[i]) {
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
	if (party == ALICE)
		cout << "Setup time: " << circ_exec->total_time << "ms" << endl;
	else
	 	cout << "Online time: " << circ_exec->total_time << "ms" << endl;
}
