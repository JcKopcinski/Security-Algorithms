#include <iostream>
#include <fstream> 
#include <cstdint>
#include <array>

#define SHIFT_MASK 0xC081
using namespace std;

uint8_t set_odd_parity(uint8_t byte){
	int ones = 0;
	for (int i = 1; i < 8; i++){ //count the number of ones
		if(byte & (1u << i)) ++ones;
	}

	byte = (byte & 0xFEu) | ((ones%2) == 0 ? 1u : 0u);
	return byte;
}

uint64_t generate_random_key64_oddparity(){
	array<uint8_t, 8>key{};

	ifstream urandom("/dev/urandom", ios::in | ios::binary);
	if(!urandom){
		cerr << "Failed to open /dev/urandom\n";
		return 0;
	}

	//fill the key with the random num
	urandom.read(reinterpret_cast<char *>(key.data()), key.size());
	if(urandom.gcount() != static_cast<std::streamsize>(key.size())){
		cerr << "Short read from /dev/urandom\n";
		return 0;
	}

	for(auto& byte : key){
		byte = set_odd_parity(byte);
	}
	
	//convert key to uint64_t
	uint64_t result = 0;
	for(size_t i = 0; i < 8; i++){
		result = (result << 8) | static_cast<uint64_t>(key[i]);
	}

	return result;
}

uint32_t left_circular_shift28(uint32_t value, int k){
	value &= 0x0FFFFFFFu;
	return ((value << k) | (value >> (28 - k))) & 0x0FFFFFFFu;
}


//---------------------------SUBKEYS AND KEY SCHEDULER-----------------
uint64_t permuted_choice2(uint32_t& C, uint32_t& D, int round){

	int shift_amount = ((SHIFT_MASK >> (15 - round)) & 1) ?  1 : 2;
	C = left_circular_shift28(C, shift_amount);
	D = left_circular_shift28(D, shift_amount);

	uint64_t combined = (static_cast<uint64_t>(C) << 28) | D;
	
	constexpr uint8_t PC2_map[48] = {
        13,16,10,23, 0, 4,
         2,27,14, 5,20, 9,
        22,18,11, 3,25, 7,
        15, 6,26,19,12, 1,
        40,51,30,36,46,54,
        29,39,50,44,32,47,
        43,48,38,55,33,52,
        45,41,49,35,28,31
	};

	uint64_t subkey = 0;
	for (int i = 0; i < 48; i++){
		subkey <<= 1;
		subkey |= (combined >> (55 - PC2_map[i])) & 1ULL;
	}

	return subkey;
}

std::pair<uint32_t, uint32_t> permuted_choice1(uint64_t key){

	uint64_t key56 = 0;
	constexpr uint8_t PC1_map[56] = {
    56, 48, 40, 32, 24, 16,  8,
     0, 57, 49, 41, 33, 25, 17,
     9,  1, 58, 50, 42, 34, 26,
    18, 10,  2, 59, 51, 43, 35,
    62, 54, 46, 38, 30, 22, 14,
     6, 61, 53, 45, 37, 29, 21,
    13,  5, 60, 52, 44, 36, 28,
    20, 12,  4, 27, 19, 11,  3
	};

	for(int i = 0; i < 56; i++){
		key56 <<= 1;
		key56 |= (key >> (63 - PC1_map[i])) & 1ULL;
	}

	uint32_t C0 = static_cast<uint32_t>((key56 >> 28) & 0x0FFFFFFFULL);
	uint32_t D0 = static_cast<uint32_t>(key56 & 0x0FFFFFFFULL);

	return {C0, D0};

}

//E Expansion Permutation (32bit -> 48bit)
uint64_t DES_expansion_permutation(uint32_t R){
    constexpr uint8_t E[48] = {
        31, 0, 1, 2, 3, 4,
         3, 4, 5, 6, 7, 8,
         7, 8, 9,10,11,12,
        11,12,13,14,15,16,
        15,16,17,18,19,20,
        19,20,21,22,23,24,
        23,24,25,26,27,28,
        27,28,29,30,31, 0
    };

	uint64_t out = 0;
	for(int i = 0; i < 48; i++){
		out <<= 1;
		out |= (static_cast<uint64_t>(R >> (31 - E[i])) & 1ULL);
	}

	return out;
}

//Side Permutations
static uint64_t initial_permutation(uint64_t block){

	    constexpr uint8_t IP[64] = {
        57,49,41,33,25,17, 9, 1,
        59,51,43,35,27,19,11, 3,
        61,53,45,37,29,21,13, 5,
        63,55,47,39,31,23,15, 7,
        56,48,40,32,24,16, 8, 0,
        58,50,42,34,26,18,10, 2,
        60,52,44,36,28,20,12, 4,
        62,54,46,38,30,22,14, 6
    };

	uint64_t out = 0;

	for(int i = 0; i< 64; i++){
		out <<= 1u;
		out |= (block >> (63 - IP[i])) & 1ULL;
	}

	return out;
}

static uint64_t final_permutation(uint64_t block){
    constexpr uint8_t FP[64] = {
        39, 7,47,15,55,23,63,31,
        38, 6,46,14,54,22,62,30,
        37, 5,45,13,53,21,61,29,
        36, 4,44,12,52,20,60,28,
        35, 3,43,11,51,19,59,27,
        34, 2,42,10,50,18,58,26,
        33, 1,41, 9,49,17,57,25,
        32, 0,40, 8,48,16,56,24
    };
    uint64_t out = 0;
    for (int i = 0; i < 64; ++i){
        out <<= 1;
        out |= (block >> (63 - FP[i])) & 1ULL;
    }
    return out;
}

// -------------------- S-boxes and P permutation --------------------
//8 S-boxes: S[box][row][col] (row from bits b5,b0; col from b4..b1)
static constexpr uint8_t SBOX[8][4][16] = {
{ // S1
 {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7},
 {0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8},
 {4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0},
 {15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13}
},
{ // S2
 {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10},
 {3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5},
 {0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15},
 {13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9}
},
{ // S3
 {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8},
 {13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1},
 {13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7},
 {1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12}
},
{ // S4
 {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15},
 {13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9},
 {10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4},
 {3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14}
},
{ // S5
 {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9},
 {14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6},
 {4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14},
 {11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3}
},
{ // S6
 {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11},
 {10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8},
 {9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6},
 {4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13}
},
{ // S7
 {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1},
 {13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6},
 {1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2},
 {6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12}
},
{ // S8
 {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7},
 {1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2},
 {7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8},
 {2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}
}
};

static constexpr uint8_t PBOX[32] = {
    15, 6,19,20,28,11,27,16,
     0,14,22,25, 4,17,30, 9,
     1, 7,23,13,31,26, 2, 8,
    18,12,29, 5,21,10, 3,24
};

static uint32_t des_feistel(uint32_t R, uint64_t subkey48){
	uint64_t x = DES_expansion_permutation(R) ^ (subkey48 * 0xFFFFFFFFFFFFULL);

	//SBoxes
	uint32_t s_out = 0;
	for (int box = 0; box < 8; ++box){
        uint8_t chunk = static_cast<uint8_t>((x >> (42 - 6*box)) & 0x3F); // top group first
        uint8_t row = static_cast<uint8_t>(((chunk & 0x20) >> 4) | (chunk & 0x01)); // b5b0
        uint8_t col = static_cast<uint8_t>((chunk >> 1) & 0x0F);                    // b4..b1
        uint8_t val = SBOX[box][row][col];
        s_out = (s_out << 4) | val;
    }
	
	//P permutation
	uint32_t pout = 0;
	for(int i = 0; i < 32; i++){
		pout <<= 1;
		pout |= (s_out >> (31 - PBOX[i])) & 1u;
	}

	return pout;
}

static inline void des_make_subkeys(uint64_t key64, uint64_t subkeys[16]){
	auto [C, D] = permuted_choice1(key64);
	for (int r = 0; r < 16; r++){
		subkeys[r] = permuted_choice2(C, D, r);
	}
}

static uint64_t des_encrypt_block(uint64_t block, const uint64_t subkeys[16]){
	uint64_t b = initial_permutation(block);
	//split the block
	uint32_t L = static_cast<uint32_t>(b >> 32);
	uint32_t R = static_cast<uint32_t>(b & 0xFFFFFFFFULL);

	for(int r = 0; r < 16; r++){
		uint32_t f = des_feistel(R, subkeys[r]);
		uint32_t newL = R;
		uint32_t newR = L ^ f;
		L = newL; R = newR;
	}

	//recombine
	uint64_t rout = (static_cast<uint64_t>(R) << 32) | L; //note, this performs a final swap
	return final_permutation(rout);
}

int main(int argc, char **argv){

	uint64_t key;

	if(argc != 2){
		cerr << "Usage: ./DES_original <file.txt>\n";
		return 1;
	}


	ifstream inputfile(argv[1]);
	if(!inputfile){
		throw runtime_error(string("Error Opening file: ") + argv[1]);
	}

	//generate out random key to be used for encryption
	key = generate_random_key64_oddparity();
	
	//May need to save the key here for use in decrypting the file
	
	//Begin encryption here
	
	inputfile.close();
	return 0;
}
