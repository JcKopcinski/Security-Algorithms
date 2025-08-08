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
//
//

#include <iostream>
#include <fstream>

using namespace std;

uint8_t set_odd_parity(uint8_t byte){
	int ones = 0;
	for (int i=0; i < 7; i++){ //count the number of ones
		if(byte & (1 << i)) ++ones;
	}

	return byte & 0xFE | ((ones%2) == 0 ? 1 : 0);
}

uint64_t generate_random_key64(){
	uint8_t key[8];

	ifstream urandom("/dev/urandom", ios::in | ios::binary);
	if(!urandom){
		cerr << "Failed to open /dev/urandom\n";
	}

	//fill the key with the random num
	urandom.read(reinterpret_cast<char *>(key), sizeof(key));
	urandom.close();

	for(int i = 0; i < 8; i++){
		key[i] = set_odd_parity(key[i]);
	}
}

int main(int argc, char **argv){

	if(argc != 2){
		cerr << "Usage: ./DES_original <file.txt>\n";
		return 1;
	}


	ifstream inputfile(argv[1]);
	if(!inputfile){
		throw runtime_error(string("Error Opening file: ") + argv[1]);
	}

	inputfile.close();
	return 0;
}
