#ifndef EMP_LOWMC_H__
#define EMP_LOWMC_H__

#include <bitset>
#include <vector>

#include "GC/bit.h"
#include "GC/integer.h"
#include <fmt/core.h>


namespace sci {

const unsigned numofboxes = 1;    // Number of Sboxes
const unsigned blocksize = 64;   // Block size in bits
const unsigned keysize = 128; // Key size in bits
const unsigned rounds = 184; // Number of rounds

const unsigned identitysize = blocksize - 3*numofboxes;
                  // Size of the identity part in the Sbox layer

#define secret_block Integer // Store messages and states
#define secret_keyblock Integer

typedef std::bitset<blocksize> block; // Store messages and states
typedef std::bitset<keysize> keyblock;

template <size_t _Nb>
Integer share(std::bitset<_Nb> b, int party=PUBLIC) {
    Integer result(_Nb, 0); 
    for (int i = 0; i < b.size(); i++) {
        result[i] = Bit(b[i], party);
    }
    return result;
}

class LowMC {
public:
    LowMC (keyblock k, int party=PUBLIC) {
        key = share(k, party);
        instantiate_LowMC();
        keyschedule();   
    };
    LowMC (secret_keyblock k) {
        key = k;
        instantiate_LowMC();
        keyschedule();   
    };

    secret_block encrypt (const secret_block message);
    std::vector<secret_block> encrypt (const std::vector<secret_block> &message);
    void set_key (keyblock k, int party=PUBLIC);
    void set_key (secret_keyblock k);

    void print_matrices();

    void verify_sbox();

private:
// LowMC private data members //
    // // The Sbox and its inverse    
    // const std::vector<unsigned> Sbox =
    //     {0x00, 0x01, 0x03, 0x06, 0x07, 0x04, 0x05, 0x02};
    // const std::vector<unsigned> invSbox =
    //     {0x00, 0x01, 0x07, 0x02, 0x05, 0x06, 0x03, 0x04};

    // Compute with the lowest 3 bits of message
    secret_block Sbox(secret_block message) {
        assert(message.size() >= 3);
        auto &a = message[2];
        auto &b = message[1];
        auto &c = message[0];
        secret_block res(blocksize, 0);
        res[2] = a ^ (b & c);
        res[1] = a ^ b ^ (a & c);
        res[0] = a ^ b ^ c ^ (a & b);
        return res;
    }
    
    void Sbox(std::vector<secret_block> &messages, size_t offset) {
        size_t len = messages.size();
        a.resize(len);
        b.resize(len);
        c.resize(len);
        for (int i = 0; i < len; i++) {
            auto& message = messages[i];
            a[i] = message[offset+2];
            b[i] = message[offset+1];
            c[i] = message[offset+0];
        }
        auto bc = Bit::batchAnd(b, c);
        auto ca = Bit::batchAnd(c, a);
        auto ab = Bit::batchAnd(a, b);
        for (int i = 0; i < len; i++) {
            auto& message = messages[i];
            message[offset+2] = a[i] ^ bc[i];
            message[offset+1] = a[i] ^ b[i] ^ ca[i];
            message[offset+0] = a[i] ^ b[i] ^ c[i] ^ ab[i];
        }
    }

    std::vector<Bit> a, b, c;

    std::vector<std::vector<block>> LinMatrices;
        // Stores the binary matrices for each round
    // std::vector<std::vector<block>> invLinMatrices;
        // Stores the inverses of LinMatrices
    std::vector<secret_block> roundconstants;
        // Stores the round constants
    secret_keyblock key;
        //Stores the master key
    std::vector<std::vector<keyblock>> KeyMatrices;
        // Stores the matrices that generate the round keys
    std::vector<secret_block> roundkeys;
        // Stores the round keys
    
// LowMC private functions //
    secret_block Substitution (const secret_block message);
    void Substitution (std::vector<secret_block> &message);
        // The substitution layer

    secret_block MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const secret_block message);    
    std::vector<secret_block> MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const std::vector<secret_block> &message);    
        // For the linear layer
    secret_block MultiplyWithGF2Matrix_Key
        (const std::vector<keyblock> &matrix, const secret_keyblock k);
        // For generating the round keys

    void keyschedule ();
        //Creates the round keys from the master key

    void instantiate_LowMC ();
        //Fills the matrices and roundconstants with pseudorandom bits 
   
// Binary matrix functions //   
    unsigned rank_of_Matrix (const std::vector<block> &matrix);
    unsigned rank_of_Matrix_Key (const std::vector<keyblock> &matrix);
    // std::vector<block> invert_Matrix (const std::vector<block> matrix);

// Random bits functions //
    block getrandblock ();
    keyblock getrandkeyblock ();
    bool  getrandbit ();

};

} // namespace sci

#endif