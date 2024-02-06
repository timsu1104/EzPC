#include "GC/emp-sh2pc.h"
#include <iostream>
#include <chrono>
#include <ratio>
#include <ctime>
#include <fmt/core.h>

using namespace sci;
using namespace std;

int party, port = 8000, iters = 512;
int bw_x = 32;
NetIO *io_gc;

void test_swap() {
	Integer A(bw_x, rand() % (1ULL << 32), ALICE);
	Integer B(bw_x, rand() % (1ULL << 32), BOB);
	
	cout << "GTGate" << endl;
    auto comm_start = io_gc->counter;
	auto time_start = high_resolution_clock::now();

	auto sel = A > B;

	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
    cout << "elapsed " << time_span * 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) << " Bytes" << endl;
	
	cout << "CondSwap" << endl;
    comm_start = io_gc->counter;
	time_start = high_resolution_clock::now();

	swap(sel, A, B);

	time_end = high_resolution_clock::now();
	time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
    cout << "elapsed " << time_span * 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) << " Bytes" << endl;

}

int main(int argc, char **argv) {
	party = atoi(argv[1]);
	io_gc = new NetIO(party == ALICE ? nullptr : "127.0.0.1",
						port + GC_PORT_OFFSET, true);

	setup_semi_honest(io_gc, party);
	test_swap();
}
