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

int main(){
	cout << "What did you eat?";
	string food;
	cin >> food;

	ofstream file("testtext.txt");
	file << food;
	file.close()

}
