#include "GC/emp-sh2pc.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <ratio>
#include <ctime>
#include <set>
#include <fmt/core.h>

using namespace sci;
using std::cout, std::endl;

int party, port = 8000, batch_size = 256;
int bitlength = 32;
NetIO *io_gc;

inline void show(int *x, int size, string name) {
  cout << name << ": ";
  for (int i=0; i<size; i++) {
    cout << x[i] << " ";
  }
  cout << endl;
}

void test_remapping() {
	int N = 1 << 16;
	int bucket_size = 3 * batch_size / 2;

	int* in = new int[batch_size];
	int* indup = new int[batch_size];
	int* resp = new int[batch_size];
	std::map<int, int> response_map; 
	Integer *in_shared = new Integer[batch_size];
	Integer *indup_shared = new Integer[batch_size];
	Integer *resp_shared = new Integer[bucket_size];

	int* cuckoo_map = new int[batch_size]();
	std::set<int> outofrange;
	
	for (int j = 0, m = batch_size; j < bucket_size; ++j) {
		if (rand() % (bucket_size - j) < m) {
			cuckoo_map[batch_size - m] = j;
			m--;
		} else {
			outofrange.insert(j);
		}
	}

	assert(outofrange.size() == bucket_size - batch_size);
	
// Now specify Alice's input

// Now specify Bob's input

	for(int i = 0; i < batch_size; ++i) {
		in[i] = rand()%(batch_size >> 1);
		response_map[in[i]] = (uint32_t) rand() % (1ULL << bitlength);
		in_shared[i] = Integer(bitlength, in[i], BOB);
	}

	memcpy(indup, in, batch_size * sizeof(uint32_t));
	std::vector<uint32_t> v(indup, indup + batch_size);
    std::set<int> duplicates;
    auto it = std::remove_if(std::begin(v), std::end(v), [&duplicates](int i) {
        return !duplicates.insert(i).second;
    });
    size_t n = std::distance(std::begin(v), it);

	uint32_t count = 0;
    for (size_t i = 0; i < batch_size; i++) {
		if (i < n) {
			indup[i] = v[i];
		} else {
			indup[i] = N + count + 1;
			count++;
		}
	}
	std::sort(indup, indup+batch_size); 
    for (size_t i = 0; i < batch_size; i++) {
		if (i < n) {
			resp[i] = response_map[indup[i]];
		} else {
			resp[i] = 0;
		}
		indup_shared[i] = Integer(bitlength, indup[i], BOB);
		resp_shared[cuckoo_map[i]] = Integer(bitlength, resp[i], BOB);
	}
	
	for (auto j: outofrange) {
		resp_shared[j] = Integer(bitlength, 0, BOB);
	}
	
    auto comm_start = io_gc->counter;
	auto time_start = high_resolution_clock::now();
	
	auto constantArray = getConstantArray(batch_size<<1, bitlength);
	auto res = remap(in_shared, indup_shared, resp_shared, batch_size, bucket_size, N, bitlength, cuckoo_map, constantArray);

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
	for(int i = 0; i < batch_size; ++i) {
		auto result = res[i].reveal<int32_t>();
		if (response_map[in[i]] != result) {
			error(fmt::format("{}-th element: {} != {}!", i, response_map[in[i]], result).c_str());
		}
	}
	cout << "Test passed" << endl;

	delete[] in;
	delete [] indup;
	delete [] cuckoo_map;
	delete [] resp;
	delete[] res;
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

	auto time_start = high_resolution_clock::now();
	setup_semi_honest(io_gc, party);
	auto time_end = high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start).count();
	cout << "General setup: elapsed " << time_span * 1000 << " ms." << endl;
	test_remapping();
	cout << "# AND gates: " << circ_exec->num_and() << endl;
	io_gc->flush();
}
