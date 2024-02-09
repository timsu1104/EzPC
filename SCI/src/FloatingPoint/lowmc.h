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

#define secret_block BoolArray // Store messages and states
#define secret_keyblock BoolArray

typedef std::bitset<blocksize> block; // Store messages and states
typedef std::bitset<keysize> keyblock;

template <size_t _Nb>
BoolArray share(std::bitset<_Nb> b, BoolOp* op, int party=sci::PUBLIC) {
    uint8_t* buffer = new uint8_t[_Nb];
    for (int i = 0; i < _Nb; i++) {
        buffer[i] = b[i];
    }
    auto result = op->input(party, _Nb, buffer);
    return op->input(party, _Nb, buffer);
}

class LowMC_bool {
public:
    LowMC_bool (keyblock k, BoolOp* op, int party=sci::PUBLIC) : op(op) {
        key = share(k, op, party);
        instantiate_LowMC_bool();
        keyschedule();   
    };
    LowMC_bool (secret_keyblock k, BoolOp* op) : key(k), op(op) {
        instantiate_LowMC_bool();
        keyschedule();   
    };

    std::vector<secret_block> encrypt (const std::vector<secret_block> &message);
    void set_key (keyblock k, int party=sci::PUBLIC);
    void set_key (secret_keyblock k);

private:
// LowMC_bool private data members //

    // Compute with the 3 bits of message (offset, offset+1, offset+2)
    void Sbox(std::vector<secret_block> &messages, size_t offset) {
        size_t len = messages.size();
        a = BoolArray(sci::BOB, len);
        b = BoolArray(sci::BOB, len);
        c = BoolArray(sci::BOB, len);
        for (int i = 0; i < len; i++) {
            auto& message = messages[i];
            a[i] = message[offset+2];
            b[i] = message[offset+1];
            c[i] = message[offset+0];
        }
        auto bc = op->AND(b, c);
        auto ca = op->AND(c, a);
        auto ab = op->AND(a, b);
        for (int i = 0; i < len; i++) {
            auto& message = messages[i];
            message[offset+2] = a[i] ^ bc[i];
            message[offset+1] = a[i] ^ b[i] ^ ca[i];
            message[offset+0] = a[i] ^ b[i] ^ c[i] ^ ab[i];
        }
    }

    BoolArray a, b, c;
    BoolOp* op;

    std::vector<std::vector<block>> LinMatrices;
        // Stores the binary matrices for each round
    std::vector<secret_block> roundconstants;
        // Stores the round constants
    secret_keyblock key;
        //Stores the master key
    std::vector<std::vector<keyblock>> KeyMatrices;
        // Stores the matrices that generate the round keys
    std::vector<secret_block> roundkeys;
        // Stores the round keys
    
// LowMC_bool private functions //
    void Substitution (std::vector<secret_block> &message);
        // The substitution layer

    std::vector<secret_block> MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const std::vector<secret_block> &message);    
        // For the linear layer
    secret_block MultiplyWithGF2Matrix_Key
        (const std::vector<keyblock> &matrix, const secret_keyblock k);
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