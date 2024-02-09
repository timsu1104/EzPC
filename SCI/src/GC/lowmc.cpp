#include <sys/types.h>
#include <vector>
#include <iostream>
#include <algorithm>

#include "lowmc.h"

namespace sci {

Integer count(Integer x) {
    Integer temp(x.size(), 0);
    Integer cnt(x.size(), 0);
    for (int i = 0;i < x.size(); i++) {
        temp[0] = x[i];
        cnt = cnt + temp;
    }
    return cnt;
}

/////////////////////////////
//     LowMC functions     //
/////////////////////////////

secret_block LowMC::encrypt (const secret_block message) {
    secret_block c = message ^ roundkeys[0];
    for (unsigned r = 1; r <= rounds; ++r) {
        c =  Substitution(c);
        c =  MultiplyWithGF2Matrix(LinMatrices[r-1], c);
        c = c ^ roundconstants[r-1];
        c = c ^ roundkeys[r];
        return c;
    }
    return c;
}

std::vector<secret_block> LowMC::encrypt (const std::vector<secret_block> &message) {
    auto len = message.size();
    std::vector<secret_block> c(len);
    for (unsigned l = 0; l < len; l++) {
        c[l] = message[l] ^ roundkeys[0];
    }
    for (unsigned r = 1; r <= rounds; ++r) {
        Substitution(c);
        c =  MultiplyWithGF2Matrix(LinMatrices[r-1], c);
        for (int l = 0; l < len; l++) 
            c[l] = c[l] ^ roundconstants[r-1] ^ roundkeys[r];
    }
    return c;
}

void LowMC::set_key (keyblock k, int party) {
    key = share(k, party);
    keyschedule();
}

void LowMC::set_key (secret_keyblock k) {
    key = k;
    keyschedule();
}

void LowMC::print_matrices() {
    std::cout << "LowMC matrices and constants" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Block size: " << blocksize << std::endl;
    std::cout << "Key size: " << keysize << std::endl;
    std::cout << "Rounds: " << rounds << std::endl;
    std::cout << std::endl;

    std::cout << "Linear layer matrices" << std::endl;
    std::cout << "---------------------" << std::endl;
    for (unsigned r = 1; r <= rounds; ++r) {
        std::cout << "Linear layer " << r << ":" << std::endl;
        for (auto row: LinMatrices[r-1]) {
            std::cout << "[";
            for (unsigned i = 0; i < blocksize; ++i) {
                std::cout << row[i];
                if (i != blocksize - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "]" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "Round constants" << std::endl;
    std::cout << "---------------------" << std::endl;
    for (unsigned r = 1; r <= rounds; ++r) {
        std::cout << "Round constant " << r << ":" << std::endl;
        std::cout << "[";
        for (unsigned i = 0; i < blocksize; ++i) {
            std::cout << roundconstants[r-1][i].reveal();
            if (i != blocksize - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "Round key matrices" << std::endl;
    std::cout << "---------------------" << std::endl;
    for (unsigned r = 0; r <= rounds; ++r) {
        std::cout << "Round key matrix " << r << ":" << std::endl;
        for (auto row: KeyMatrices[r]) {
            std::cout << "[";
            for (unsigned i = 0; i < keysize; ++i) {
                std::cout << row[i];
                if (i != keysize - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "]" << std::endl;
        }
        if (r != rounds) {
            std::cout << std::endl;
        }
    }
}

void LowMC::verify_sbox() {
    const std::vector<unsigned> sbox = {0x00, 0x01, 0x03, 0x06, 0x07, 0x04, 0x05, 0x02};
    for (int i = 0; i < 8; i++) {
        assert(sbox[i] == Sbox(Integer(3, i)).reveal<unsigned>());
    }
}

/////////////////////////////
// LowMC private functions //
/////////////////////////////


secret_block LowMC::Substitution (const secret_block message) {
    //Get the identity part of the message
    secret_block temp = (message >> 3*numofboxes);
    //Get the rest through the Sboxes
    for (unsigned i = 1; i <= numofboxes; ++i) {
        auto buf = message >> 3*(numofboxes-i);
        auto sbox = Sbox(buf);
        temp = (temp << 3) ^ sbox;
    }
    return temp;
}

void LowMC::Substitution (std::vector<secret_block> &message) {
    for (unsigned i = 0; i < numofboxes; ++i) {
        Sbox(message, 3*i); 
    }
}

secret_block LowMC::MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const secret_block message) {
    secret_block temp(blocksize, 0);
    for (unsigned i = 0; i < blocksize; ++i) {
        for (unsigned j = 0; j < blocksize; ++j) {
            if (matrix[i][j]) {
                temp[i] = temp[i] ^ message[j];
            }
        }
    }
    return temp;
}


std::vector<secret_block> LowMC::MultiplyWithGF2Matrix
        (const std::vector<block> &matrix, const std::vector<secret_block> &message) {
    std::vector<secret_block> res(message.size(), Integer(blocksize, 0));
    for (unsigned i = 0; i < blocksize; ++i) {
        for (unsigned j = 0; j < blocksize; ++j) {
            if (matrix[i][j]) {
                for (unsigned l = 0; l < res.size(); ++l) {
                    res[l][i] = res[l][i] ^ message[l][j];
                }
            }
        }
    }
    return res;
}


secret_block LowMC::MultiplyWithGF2Matrix_Key
        (const std::vector<keyblock> &matrix, const secret_keyblock k) {
    secret_block temp(blocksize, 0);
    for (unsigned i = 0; i < blocksize; ++i) {
        for (unsigned j = 0; j < blocksize; ++j) {
            if (matrix[i][j]) {
                temp[i] = temp[i] ^ k[j];
            }
        }
    }
    return temp;
}

void LowMC::keyschedule () {
    roundkeys.clear();
    for (unsigned r = 0; r <= rounds; ++r) {
        roundkeys.push_back( MultiplyWithGF2Matrix_Key (KeyMatrices[r], key) );
    }
    return;
}


void LowMC::instantiate_LowMC () {
    // Create LinMatrices and invLinMatrices
    LinMatrices.clear();
    // invLinMatrices.clear();
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
        // invLinMatrices.push_back(invert_Matrix(mat));
    }

    // Create roundconstants
    roundconstants.clear();
    for (unsigned r = 0; r < rounds; ++r) {
        roundconstants.push_back( share(getrandblock ()) );
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


unsigned LowMC::rank_of_Matrix (const std::vector<block> &matrix) {
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


unsigned LowMC::rank_of_Matrix_Key (const std::vector<keyblock> &matrix) {
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


// std::vector<block> LowMC::invert_Matrix (const std::vector<block> matrix) {
//     std::vector<block> mat; //Copy of the matrix 
//     for (auto u : matrix) {
//         mat.push_back(u);
//     }
//     std::vector<block> invmat(blocksize, 0); //To hold the inverted matrix
//     for (unsigned i = 0; i < blocksize; ++i) {
//         invmat[i][i] = 1;
//     }

//     unsigned size = mat[0].size();
//     //Transform to upper triangular matrix
//     unsigned row = 0;
//     for (unsigned col = 0; col < size; ++col) {
//         if ( !mat[row][col] ) {
//             unsigned r = row+1;
//             while (r < mat.size() && !mat[r][col]) {
//                 ++r;
//             }
//             if (r >= mat.size()) {
//                 continue;
//             } else {
//                 auto temp = mat[row];
//                 mat[row] = mat[r];
//                 mat[r] = temp;
//                 temp = invmat[row];
//                 invmat[row] = invmat[r];
//                 invmat[r] = temp;
//             }
//         }
//         for (unsigned i = row+1; i < mat.size(); ++i) {
//             if ( mat[i][col] ) {
//                 mat[i] ^= mat[row];
//                 invmat[i] ^= invmat[row];
//             }
//         }
//         ++row;
//     }

//     //Transform to identity matrix
//     for (unsigned col = size; col > 0; --col) {
//         for (unsigned r = 0; r < col-1; ++r) {
//             if (mat[r][col-1]) {
//                 mat[r] ^= mat[col-1];
//                 invmat[r] ^= invmat[col-1];
//             }
//         }
//     }

//     return invmat;
// }

///////////////////////
// Pseudorandom bits //
///////////////////////


block LowMC::getrandblock () {
    block tmp = 0;
    for (unsigned i = 0; i < blocksize; ++i) tmp[i] = getrandbit ();
    return tmp;
}

keyblock LowMC::getrandkeyblock () {
    keyblock tmp = 0;
    for (unsigned i = 0; i < keysize; ++i) tmp[i] = getrandbit ();
    return tmp;
}


// Uses the Grain LSFR as self-shrinking generator to create pseudorandom bits
// Is initialized with the all 1s state
// The first 160 bits are thrown away
bool LowMC::getrandbit () {
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

} // namespace sci