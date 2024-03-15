#include "GC/emp-sh2pc.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fmt/core.h>

using namespace sci;
using std::cout, std::endl, std::vector;

int party, port = 8000, batch_size = 384;
int bitlength = 16;
int w = 3;
NetIO *io_gc;

void test_match() {

	std::vector<std::vector<int>> in(w);
	std::vector<int> sel(batch_size);
	IntegerArray selx(batch_size);
	IntegerArray result(batch_size);
	vector<IntegerArray> shrin(w);
	vector<IntegerArray> shrout(w);
	std::map<int, int> lut;

	for (int i = 0; i < (1 << bitlength); i ++) {
		lut[i] = rand() % (1 << bitlength);
	}
	
	for (int j = 0; j < w; ++j) {
		in[j].resize(batch_size);
		shrin[j].resize(batch_size);
		shrout[j].resize(batch_size);
		for(int i = 0; i < batch_size; ++i) {
			in[j][i] = rand()%(1 << bitlength);
			shrin[j][i] = Integer(bitlength, in[j][i], BOB);
			shrout[j][i] = Integer(bitlength, lut[in[j][i]], BOB);
		}
	}

	for(int i = 0; i < batch_size; ++i) {
		sel[i] = rand() % w;
		selx[i] = shrin[sel[i]][i];
	}
	
	cout << BLUE << "Match" << RESET << endl;
    auto comm_start = io_gc->counter;
	auto time_start = clock_start();
	
	auto zero = Integer(bitlength, 0);
	for(int i = 0; i < batch_size; ++i) {
		result[i] = zero;
		for (int j = 0; j < w; ++j) {
			result[i] = result[i] + If(selx[i] == shrin[j][i], shrout[j][i], zero);
		}
	}

	auto time_span = time_from(time_start);
    cout << "elapsed " << time_span / 1000 << " ms." << endl;
    cout << "sent " << (io_gc->counter - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;

	// Verify
	for(int i = 0; i < batch_size; ++i) {
		if (result[i].reveal<uint32_t>() != lut[in[sel[i]][i]]) {
			error(fmt::format("{} not match!", i).c_str());
		}
	}
	cout << GREEN << "[Match] Test passed" << RESET << endl;

}

int main(int argc, char **argv) {
	
	ArgMapping amap;
	amap.arg("r", party, "Role of party: ALICE = 1; BOB = 2");
	amap.arg("p", port, "Port Number");
	amap.arg("s", batch_size, "number of total elements");
	amap.arg("l", bitlength, "bitlength of inputs");
	amap.parse(argc, argv);
	io_gc = new NetIO(party == ALICE ? nullptr : "127.0.0.1",
						port + GC_PORT_OFFSET, true);

	auto time_start = clock_start(); 
	setup_semi_honest(io_gc, party);
	auto time_span = time_from(time_start);
	cout << "General setup: elapsed " << time_span / 1000 << " ms." << endl;
	test_match();
	io_gc->flush();
}
