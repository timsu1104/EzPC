#ifndef BOOL_LowMC_bool_H__
#define BOOL_LowMC_bool_H__

#include <bitset>
#include <cstdint>
#include <vector>

#include "FloatingPoint/bool-data.h"
#include <fmt/core.h>

const unsigned numofboxes = 1;    // Number of Sboxes
const unsigned blocksize = 64;   // Block size in bits
const unsigned keysize = 128; // Key size in bits
const unsigned rounds = 184; // Number of rounds

const unsigned identitysize = blocksize - 3*numofboxes;
                  // Size of the identity part in the Sbox layer


typedef std::bitset<blocksize> block; // Store messages and states
typedef std::bitset<keysize> keyblock;
typedef std::array<BoolArray, blocksize> secret_block_bool; // Store messages and states
typedef std::array<BoolArray, keysize> secret_keyblock_bool;

template <size_t _Nb>
std::array<BoolArray, _Nb> share(std::bitset<_Nb> b, BoolOp* op, uint32_t nvals, int party=sci::PUBLIC) {
    std::array<BoolArray, _Nb> result;
    BoolArray zero = op->input(party, nvals, (uint8_t)0);
    BoolArray one = op->input(party, nvals, 1);
    for (int i = 0; i < _Nb; i++) {
        result[i] = b[i] ? one : zero;
    }
    return result;
}

class LowMC_bool {
public:
    uint32_t nvals;
    LowMC_bool (keyblock k, BoolOp* op, uint32_t nvals=1, int party=sci::PUBLIC) : op(op), nvals(nvals) {
        key = share(k, op, nvals, party);
        instantiate_LowMC_bool();
        keyschedule();   
    };

    secret_block_bool encrypt (const secret_block_bool message);
    void set_key (keyblock k, int party=sci::PUBLIC);
    void set_key (secret_keyblock_bool k);

private:
// LowMC_bool private data members //

    // Compute with the 3 bits of message (offset, offset+1, offset+2)
    void Sbox(secret_block_bool &messages, size_t offset) {
        auto& a = messages[offset+2];
        auto& b = messages[offset+1];
        auto& c = messages[offset+0];
        auto bc = op->AND(b, c);
        auto ca = op->AND(c, a);
        auto ab = op->AND(a, b);
        messages[offset+0] = op->XOR(a, op->XOR(b, op->XOR(c, ab)));
        messages[offset+1] = op->XOR(a, op->XOR(b, ca));
        messages[offset+2] = op->XOR(a, bc);
    }

    BoolOp* op;

    std::vector<std::vector<block>> LinMatrices;
        // Stores the binary matrices for each round
    std::vector<secret_block_bool> roundconstants;
        // Stores the round constants
    secret_keyblock_bool key;
        //Stores the master key
    std::vector<std::vector<keyblock>> KeyMatrices;
        // Stores the matrices that generate the round keys
    std::vector<secret_block_bool> roundkeys;
        // Stores the round keys
    
// LowMC_bool private functions //
    void Substitution (secret_block_bool &message);
        // The substitution layer

    secret_block_bool MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const secret_block_bool message);    
        // For the linear layer
    secret_block_bool MultiplyWithGF2Matrix_Key
        (const std::vector<keyblock> &matrix, const secret_keyblock_bool k);
        // For generating the round keys

    void keyschedule ();
        //Creates the round keys from the master key

    void instantiate_LowMC_bool ();
        //Fills the matrices and roundconstants with pseudorandom bits 
   
// Binary matrix functions //   
    unsigned rank_of_Matrix (const std::vector<block> &matrix);
    unsigned rank_of_Matrix_Key (const std::vector<keyblock> &matrix);

// Random bits functions //
    block getrandblock ();
    keyblock getrandkeyblock ();
    bool  getrandbit ();

};

#endif