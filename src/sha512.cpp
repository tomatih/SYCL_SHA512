#include "sha512.hpp"
#include <stdexcept>
#include <bit>

void preprocess(const std::string& password, uint64_t* message){
    // length safety
    if(password.length() > 111){
        throw std::runtime_error("Passwords over 111 bytes are unsupported");
    }

    // fill memory for all 64 bit
    for(int i =0; i<16; i++){
        uint64_t tmp = 0;
        // for all bytes
        for(int j=0;j<8; j++){
            // calculate message byte
            size_t byte_pos = i*8 + j;
            // assume filler
            uint8_t byte = 0;
            // copy over original message
            if(byte_pos < password.length()){
                byte = password[byte_pos];
            }
            // add the minimum padding
            else if(byte_pos == password.length()){
                byte = 0b10000000;
            }
            // save as part of u64 maintaining endian
            tmp = tmp << 8 | byte;
        }
        // save to message array
        message[i] = tmp;
    }

    // encode message length
    message[15] = (uint64_t)password.length() * 8;
}

void hash_message(const uint64_t* __restrict__ message, uint64_t* hash_d, size_t index){
    // ASSUMING SINGLE CHUNK
    // init message schedule
    uint64_t w[80];
    // hash cache
    uint64_t hash[512/64];
    // offset into array
    message += index*1024/64;
    hash_d += index * 512/64;
    // copy chunk into message schedule
    std::memcpy(w, message, 16*sizeof(uint64_t));
    // extend first 16 words into the remaining 48
    #pragma unroll
    for(int i=16; i<80; i++){
        uint64_t s0 = std::rotr(w[i-15],1) ^ std::rotr(w[i-15],8) ^ (w[i-15] >> 7);
        uint64_t s1 = std::rotr(w[i-2],19) ^ std::rotr(w[i-2],61) ^ (w[i-2] >> 6);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    // init working variables
    std::memcpy(hash, initialisation_vector, 8*sizeof(uint64_t));

    // compression loop
    #pragma unroll
    for(int i=0; i<80; i++){
        uint64_t S1 = std::rotr(hash[4], 14) ^ std::rotr(hash[4], 18) ^ std::rotr(hash[4], 41);
        uint64_t ch = (hash[4] & hash[5]) ^ ((~hash[4]) & hash[6]);
        uint64_t temp1 = hash[7] + S1 + ch + round_constants[i] + w[i];
        uint64_t S0 = std::rotr(hash[0], 28) ^ std::rotr(hash[0], 34) ^ std::rotr(hash[0], 39);
        uint64_t maj = (hash[0] & hash[1]) ^ (hash[0] & hash[2]) ^ (hash[1] & hash[2]);
        uint64_t temp2 = S0 + maj;

        hash[7] = hash[6];
        hash[6] = hash[5];
        hash[5] = hash[4];
        hash[4] = hash[3] + temp1;
        hash[3] = hash[2];
        hash[2] = hash[1];
        hash[1] = hash[0];
        hash[0] = temp1 + temp2;
    }
    // update main hash
    #pragma unroll
    for(int i=0; i<8; i++){
        hash_d[i] = hash[i] + initialisation_vector[i];
    }
}