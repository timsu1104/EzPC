#include <vector>
#include <cstdlib>

#include "LowMC.h"

/////////////////////////////
//     LowMC_bool functions     //
/////////////////////////////

std::vector<secret_block> LowMC_bool::encrypt (const std::vector<secret_block> &message) {
    auto len = message.size();
    std::vector<secret_block> c(len);
    for (unsigned l = 0; l < len; l++) {
        c[l] = op->XOR(message[l], roundkeys[0]);
    }
    for (unsigned r = 1; r <= rounds; ++r) {
        Substitution(c);
        c =  MultiplyWithGF2Matrix(LinMatrices[r-1], c);
        for (int l = 0; l < len; l++)  {
            c[l] = op->XOR(c[l], roundconstants[r-1]);
            c[l] = op->XOR(c[l], roundkeys[r]);
        }
    }
    return c;
}

void LowMC_bool::set_key (keyblock k, int party) {
    key = share(k, op, party);
    keyschedule();
}

void LowMC_bool::set_key (secret_keyblock k) {
    key = k;
    keyschedule();
}

/////////////////////////////
// LowMC_bool private functions //
/////////////////////////////

void LowMC_bool::Substitution (std::vector<secret_block> &message) {
    for (unsigned i = 0; i < numofboxes; ++i) {
        Sbox(message, 3*i); 
    }
}

std::vector<secret_block> LowMC_bool::MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const std::vector<secret_block> &message) {
    size_t len = message.size();
    std::vector<secret_block> res(len, BoolArray(sci::PUBLIC, blocksize));
    for (unsigned i = 0; i < blocksize; ++i) {
        for (unsigned j = 0; j < blocksize; ++j) {
            if (matrix[i][j]) {
                for (unsigned l = 0; l < len; ++l) {
                    res[l][i] = res[l][i] ^ message[l][j];
                }
            }
        }
    }
    return res;
}


secret_block LowMC_bool::MultiplyWithGF2Matrix_Key
        (const std::vector<keyblock> &matrix, const secret_keyblock k) {
    secret_block res(sci::PUBLIC, blocksize);
    for (unsigned i = 0; i < blocksize; ++i) {
        for (unsigned j = 0; j < blocksize; ++j) {
            if (matrix[i][j]) {
                res[i] = res[i] ^ k[j];
            }
        }
    }
    return res;
}

void LowMC_bool::keyschedule () {
    roundkeys.clear();
    for (unsigned r = 0; r <= rounds; ++r) {
        roundkeys.push_back( MultiplyWithGF2Matrix_Key (KeyMatrices[r], key) );
    }
    return;
}


void LowMC_bool::instantiate_LowMC_bool () {
    // Create LinMatrices and invLinMatrices
    LinMatrices.clear();
    for (unsigned r = 0; r < rounds; ++r) {
        // Create matrix
        std::vector<block> mat;
        // Fill matrix with random bits
        do {
            mat.clear();
            for (unsigned i = 0; i < blocksize; ++i) {
                mat.push_back( getrandblock () );
            }
        // Repeat if matrix is not invertible
        } while ( rank_of_Matrix(mat) != blocksize );
        LinMatrices.push_back(mat);
    }

    // Create roundconstants
    roundconstants.clear();
    for (unsigned r = 0; r < rounds; ++r) {
        roundconstants.push_back( share(getrandblock (), op) );
    }

    // Create KeyMatrices
    KeyMatrices.clear();
    for (unsigned r = 0; r <= rounds; ++r) {
        // Create matrix
        std::vector<keyblock> mat;
        // Fill matrix with random bits
        do {
            mat.clear();
            for (unsigned i = 0; i < blocksize; ++i) {
                mat.push_back( getrandkeyblock () );
            }
        // Repeat if matrix is not of maximal rank
        } while ( rank_of_Matrix_Key(mat) < std::min(blocksize, keysize) );
        KeyMatrices.push_back(mat);
    }
    
    return;
}


/////////////////////////////
// Binary matrix functions //
/////////////////////////////


unsigned LowMC_bool::rank_of_Matrix (const std::vector<block> &matrix) {
    std::vector<block> mat; //Copy of the matrix 
    for (auto u : matrix) {
        mat.push_back(u);
    }
    unsigned size = mat[0].size();
    //Transform to upper triangular matrix
    unsigned row = 0;
    for (unsigned col = 1; col <= size; ++col) {
        if ( !mat[row][size-col] ) {
            unsigned r = row;
            while (r < mat.size() && !mat[r][size-col]) {
                ++r;
            }
            if (r >= mat.size()) {
                continue;
            } else {
                auto temp = mat[row];
                mat[row] = mat[r];
                mat[r] = temp;
            }
        }
        for (unsigned i = row+1; i < mat.size(); ++i) {
            if ( mat[i][size-col] ) mat[i] ^= mat[row];
        }
        ++row;
        if (row == size) break;
    }
    return row;
}


unsigned LowMC_bool::rank_of_Matrix_Key (const std::vector<keyblock> &matrix) {
    std::vector<keyblock> mat; //Copy of the matrix 
    for (auto u : matrix) {
        mat.push_back(u);
    }
    unsigned size = mat[0].size();
    //Transform to upper triangular matrix
    unsigned row = 0;
    for (unsigned col = 1; col <= size; ++col) {
        if ( !mat[row][size-col] ) {
            unsigned r = row;
            while (r < mat.size() && !mat[r][size-col]) {
                ++r;
            }
            if (r >= mat.size()) {
                continue;
            } else {
                auto temp = mat[row];
                mat[row] = mat[r];
                mat[r] = temp;
            }
        }
        for (unsigned i = row+1; i < mat.size(); ++i) {
            if ( mat[i][size-col] ) mat[i] ^= mat[row];
        }
        ++row;
        if (row == size) break;
    }
    return row;
}


///////////////////////
// Pseudorandom bits //
///////////////////////


block LowMC_bool::getrandblock () {
    block tmp = 0;
    for (unsigned i = 0; i < blocksize; ++i) tmp[i] = getrandbit ();
    return tmp;
}

keyblock LowMC_bool::getrandkeyblock () {
    keyblock tmp = 0;
    for (unsigned i = 0; i < keysize; ++i) tmp[i] = getrandbit ();
    return tmp;
}


// Uses the Grain LSFR as self-shrinking generator to create pseudorandom bits
// Is initialized with the all 1s state
// The first 160 bits are thrown away
bool LowMC_bool::getrandbit () {
    static std::bitset<80> state; //Keeps the 80 bit LSFR state
    bool tmp = 0;
    //If state has not been initialized yet
    if (state.none ()) {
        state.set (); //Initialize with all bits set
        //Throw the first 160 bits away
        for (unsigned i = 0; i < 160; ++i) {
            //Update the state
            tmp =  state[0] ^ state[13] ^ state[23]
                       ^ state[38] ^ state[51] ^ state[62];
            state >>= 1;
            state[79] = tmp;
        }
    }
    //choice records whether the first bit is 1 or 0.
    //The second bit is produced if the first bit is 1.
    bool choice = false;
    do {
        //Update the state
        tmp =  state[0] ^ state[13] ^ state[23]
                   ^ state[38] ^ state[51] ^ state[62];
        state >>= 1;
        state[79] = tmp;
        choice = tmp;
        tmp =  state[0] ^ state[13] ^ state[23]
                   ^ state[38] ^ state[51] ^ state[62];
        state >>= 1;
        state[79] = tmp;
    } while (! choice);
    return tmp;
}
