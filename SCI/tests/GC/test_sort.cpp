#include "GC/emp-sh2pc.h"
#include <iostream>
#include <chrono>
#include <ratio>
#include <ctime>
#include <fmt/core.h>

using namespace sci;
using std::cout, std::endl;

int party, port = 8000, iters = 512, size = 256;
int bitlength = 32;
NetIO *io_gc;

void test_sort() {
	Integer *A = new Integer[size];
	Integer *B = new Integer[size];
	Integer *res = new Integer[size];
	
	Integer *subkey = new Integer[size];

// First specify Alice's input
	for(int i = 0; i < size; ++i)
		A[i] = Integer(bitlength, rand()%(size >> 1), ALICE);

// Now specify Bob's input
	for(int i = 0; i < size; ++i)
		B[i] = Integer(bitlength, rand()%(size >> 1), BOB);
	
// Now specify public subkey
	for(int i = 0; i < size; ++i)
		subkey[i] = Integer(bitlength, size - i, PUBLIC);

//Now compute
	for(int i = 0; i < size; ++i)
		res[i] = A[i] ^ B[i];
	
    auto comm_start = io_gc->counter;
	auto time_start = high_resolution_clock::now();

	sort(res, size);

	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
    cout << "elapsed " << time_span * 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;
	// for(int i = 0; i < size; ++i)
	// 	cout << fmt::format("({}, {})", res[i].reveal<int32_t>(), subkey[i].reveal<int32_t>())<<" ";
	// cout << endl;

	delete[] A;
	delete[] B;
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

	setup_semi_honest(io_gc, party);
	test_sort();
	cout << "# AND gates: " << circ_exec->num_and() << endl;
if (party == ALICE)
		cout << "Setup time: " << circ_exec->total_time << "ms" << endl;
	else
	 	cout << "Online time: " << circ_exec->total_time << "ms" << endl;
}
