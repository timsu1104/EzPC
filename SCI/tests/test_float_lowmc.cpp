#include "FloatingPoint/bool-data.h"
#include "FloatingPoint/lowmc.h"
#include <iostream>
#include <chrono>
#include <ratio>
#include <ctime>
#include <fmt/core.h>
#include <vector>

using std::cout, std::endl;

int party, port = 8000, size = 256;
string address = "127.0.0.1";
BoolOp* op;
sci::IOPack* iopack;
sci::OTPack* otpack;

void test_lowmc() {
	std::vector<secret_block> m(size, op->inputx(sci::BOB, blocksize, 0xFFD5));
	
	auto time_start = high_resolution_clock::now();

	LowMC_bool cipher(1, op, sci::ALICE);
	
    cout << "Built cipher: elapsed " << sci::time_from(time_start) / 1000.0 << " ms." << endl;

    auto comm_start = iopack->get_comm();
	time_start = high_resolution_clock::now();

	auto res = cipher.encrypt(m);

    cout << "Encryption: elapsed " << sci::time_from(time_start) / 1000.0 << " ms." << endl;
    cout << "Encryption: sent " << (iopack->get_comm() - comm_start) / (1.0 * (1ULL << 20)) << " MB" << endl;

	// std::bitset<blocksize> ground_truth("0111010001011000111001110000111110101110010110001100011101011001");
	// for (int i=0; i<blocksize; i++) {
	// 	if (res[0][i].reveal() != ground_truth[i]) {
	// 		error(fmt::format("{}-th position not align! ", i).c_str());
	// 	}
	// }
    cout << "Test Passed!" << endl;
}

int main(int argc, char **argv) {
	
	ArgMapping amap;
	amap.arg("r", party, "Role of party: sci::ALICE = 1; sci::BOB = 2");
  	amap.arg("ip", address, "IP Address of server (ALICE)");
	amap.arg("p", port, "Port Number");
	amap.arg("s", size, "number of total elements");
	amap.parse(argc, argv);

	auto time_start = high_resolution_clock::now();
	iopack = new sci::IOPack(party, port, address);
	otpack = new sci::OTPack(iopack, party);
	op = new BoolOp(party, iopack, otpack);

	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
	cout << "General setup: elapsed " << time_span * 1000 << " ms." << endl;

	test_lowmc();
}
