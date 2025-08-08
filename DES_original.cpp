//Block size is 64 bits
//Key size is 64 bits, however only 56 is actually used by the algorithm
	//The other 8 bits are used to check for bit parity and then discarded
//Stored in 8 bytes with odd parity
//	WHAT IS ODD PARITY THOUGH...
//one bit per byte of the key is used for error detection. Nits 8, 16 .... 64 are the parity bits
//DES, by itself, is not a secure means of encryption, but must be used with a mode of operation
//	WHAT IS MODE OF OPERATION
//Decryption uses the same process as encryption, but with the keys set in reverse order
//
//DES is split intop 16 stages, or rounds, of processing
//There is also the Initial Permutation (IP) and final Permutaiton (FP), which are inverses of eachother. FP undoes IP...
//
//Before any of the rounds are processed, the block is divided into two 32 bit halves and processed alternatingly, and processed in a criss-cross pattern (Feistel Scheme). This ensure that both encrypting and decrypting are a very similar process. Thus, there is no need to separate the algorithms from one another. You jsut operate in reverse.
//
//Feistel Function of DES:
//1.The 32 bit half block taken from earlier get expanded to 48 bits, by duplicating half the bits
//
#include <iostream>
#include <fstream> #include <cstdint>
#include <array>

#define SHIFT_MASK 0xC081

using namespace std;

uint8_t set_odd_parity(uint8_t byte){
	int ones = 0;
	for (int i = 1; i < 8; i++){ //count the number of ones
		if(byte & (1 << i)) ++ones;
	}

	byte = (byte & 0xFE) | ((ones%2) == 0 ? 1 : 0);
	return byte;
}

array<uint8_t, 8> generate_random_key64_oddparity(){
	array<uint8_t, 8>key;

	ifstream urandom("/dev/urandom", ios::in | ios::binary);
	if(!urandom){
		cerr << "Failed to open /dev/urandom\n";
		
	}

	//fill the key with the random num
	urandom.read(reinterpret_cast<char *>(key.data()), key.size());
	urandom.close();

	for(auto& byte : key){
		byte = set_odd_parity(byte);
	}

	return key;
}

uint32_t left_circular_shift28(uint32_t half_subkey, int k){
	half_subkey &= 0x0FFFFFFF;
	return ((value << k) | (value >> (28 - k))) & 0x0FFFFFFF;
}

uint64_t permuted_choice(uint32_t& C, uint32_t& D, int round){

	int shift_amount = ((SHIFT_MASK >> (15 - round)) & 1) ?  1 : 2;
	C = left_circular_shift28(C, shift_amount)
	D = left_circular_shift28(D, shift_amount)

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
		subkey |= (combined >> (55 - PC2_map[i])) & 1;
	}

	return subkey;
}

pair::<uint32_t, uint32_t> call_permuted_choice1(uint64_t key){

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
		key56 |= (key >> (63 - PC1_map[i])) & 1;
	}

	uint32_t C0 = (key56 >> 28) & 0x0FFFFFFF;
	uint32_t D0 = key56 & 0x0FFFFFFF;

	return {C0, D0};

}
int main(int argc, char **argv){

	array<uint8_t, 8> key;

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
		
	//Pad the key to 64 bits...Ususally used for parity

	inputfile.close();
	return 0;
}
