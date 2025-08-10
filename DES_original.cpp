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

uint64_t generate_random_key64_oddparity(){
	array<uint8_t, 8>key;

	ifstream urandom("/dev/urandom", ios::in | ios::binary);
	if(!urandom){
		cerr << "Failed to open /dev/urandom\n";
		return 0;
	}

	//fill the key with the random num
	urandom.read(reinterpret_cast<char *>(key.data()), key.size());
	urandom.close();

	for(auto& byte : key){
		byte = set_odd_parity(byte);
	}
	
	//convert key to uint64_t
	uint64_t result = 0;
	for(size_t i = 0; i < 8; i++){
		result = (result << 8) | key[i];
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
		

	inputfile.close();
	return 0;
}
