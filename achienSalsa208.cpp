#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string>
#include <sstream>
#define ERRMESSAGE "Usage ./salsa208 <key length:64,128,256> <key in hexadecimal> <64-bit/8-byte nonce/IV in hexadecimal> <upto 1KB of plainText in hexadecimal>"
#define hexitPerInt 8
using namespace std;

unsigned int leftCircleShift(unsigned int x, int shift){
    return (x << shift) | (x >> (sizeof(x) * 8 - shift));   //circle shift function based on https://www.geeksforgeeks.org/cpp/cpp-program-to-rotate-bits-of-a-number/ on line 18 of the first code block
}
void quarterRound(unsigned int *y0, unsigned int *y1, unsigned int *y2, unsigned int *y3){
    unsigned int z0, z1, z2, z3;
    z1 = *y1 ^ leftCircleShift(*y0 + *y3, 7);
    z2 = *y2 ^ leftCircleShift(z1 + *y0, 9);
    z3 = *y3 ^ leftCircleShift(z2 + z1, 13);
    z0 = *y0 ^ leftCircleShift(z3 + z2, 18);
    *y0 = z0;
    *y1 = z1;
    *y2 = z2;
    *y3 = z3;
}
unsigned int* rowRound(unsigned int *x){
    quarterRound(&x[0],&x[1], &x[2], &x[3]);
    quarterRound(&x[5],&x[6], &x[7], &x[4]);
    quarterRound(&x[10],&x[11], &x[8], &x[9]);
    quarterRound(&x[15],&x[12], &x[13], &x[14]);
    return x;
}
unsigned int* columnRound(unsigned int *x){
    quarterRound(&x[0],&x[4], &x[8], &x[12]);
    quarterRound(&x[5],&x[9], &x[13], &x[1]);
    quarterRound(&x[10],&x[14], &x[2], &x[6]);
    quarterRound(&x[15],&x[3], &x[7], &x[11]);
    return x;
}
void doubleRound(unsigned int *x){  //tested (as a result, the rest above are tested)
    rowRound(columnRound(x));       //(wasn't tested rowRound's x[8] was a x[5], now fixed)
}
unsigned int swapEndian(unsigned int word){ //tested: byte reversal (works both ways)
    return ((word & 0xff) << 24) | ((word & 0xff00) << 8) | ((word & 0xff0000) >> 8) | ((word & 0xff000000) >> 24);
}

void salsa208KSG(unsigned int *outKeystream, unsigned short keyLength, unsigned int *key, unsigned int *nonce, unsigned long &blockCount){ //KSG = keystream generator
    union{
        unsigned int matrix[4][4];
        unsigned int keystream[16];
    } matrixKeystream;

    unsigned int originalKeystream[16];        //expansion function: inserts into 4x4 matrix/16 length array
    matrixKeystream.matrix[0][0] = 0x65787061; //expa in ascii 
    matrixKeystream.matrix[3][3] = 0x7465206B; //te k in ascii 
    matrixKeystream.matrix[1][2] = nonce[0];
    matrixKeystream.matrix[1][3] = nonce[1];
    matrixKeystream.matrix[2][0] = blockCount & 0xffffffff;
    matrixKeystream.matrix[2][1] = (blockCount & 0xffffffff00000000)>>32;
    switch (keyLength){
    case 64:
        matrixKeystream.matrix[1][1] = 0x6E642030; //nd 0 in ascii 
        matrixKeystream.matrix[2][2] = 0x382D6279; //8-by in ascii 
        matrixKeystream.matrix[0][1] = key[0];
        matrixKeystream.matrix[0][2] = key[1];
        matrixKeystream.matrix[0][3] = key[0];
        matrixKeystream.matrix[1][0] = key[1];
        matrixKeystream.matrix[2][3] = key[0];
        matrixKeystream.matrix[3][0] = key[1];
        matrixKeystream.matrix[3][1] = key[0];
        matrixKeystream.matrix[3][2] = key[1];
        break;
    case 256:
        matrixKeystream.matrix[1][1] = 0x6E642033; //nd 3 in ascii hex
        matrixKeystream.matrix[2][2] = 0x322D6279; //2-by in ascii hex
        matrixKeystream.matrix[0][1] = key[0];
        matrixKeystream.matrix[0][2] = key[1];
        matrixKeystream.matrix[0][3] = key[2];
        matrixKeystream.matrix[1][0] = key[3];
        matrixKeystream.matrix[2][3] = key[4];
        matrixKeystream.matrix[3][0] = key[5];
        matrixKeystream.matrix[3][1] = key[6];
        matrixKeystream.matrix[3][2] = key[7];
        break;
    case 128:
    default:    //128-bit key by default
        matrixKeystream.matrix[1][1] = 0x6E642031; //nd 1 in ascii hex
        matrixKeystream.matrix[2][2] = 0x362D6279; //6-by in ascii hex
        matrixKeystream.matrix[0][1] = key[0];
        matrixKeystream.matrix[0][2] = key[1];
        matrixKeystream.matrix[0][3] = key[2];
        matrixKeystream.matrix[1][0] = key[3];
        matrixKeystream.matrix[2][3] = key[0];
        matrixKeystream.matrix[3][0] = key[1];
        matrixKeystream.matrix[3][1] = key[2];
        matrixKeystream.matrix[3][2] = key[3];
    }                                                                       //to here

    for(int i = 0; i<16; i++)
        matrixKeystream.keystream[i] = swapEndian(matrixKeystream.keystream[i]);  //convert to little endian
    for(int i = 0; i<16; i++)
        originalKeystream[i]=matrixKeystream.keystream[i];  //save a copy for addition later
    for(int i = 0; i<4; i++){
        doubleRound(matrixKeystream.keystream); //perform doubleRound 4x (4x2=8 --> 20/8)
    }
    for(int i = 0; i<16; i++){
        outKeystream[i] = swapEndian(matrixKeystream.keystream[i] + originalKeystream[i]);  //undo little endian for the sum
    }
    blockCount++; //increase block counter for each block encountered (should work, need more test cases)
}

int main(int argc, char *argv[])
{
    if (argc != 5){
        cout << ERRMESSAGE << endl;
        return 1;
    }
    unsigned short keyLength = (short)stoi(argv[1]);    //test for valid key length
    if (keyLength != 64 && keyLength != 128 && keyLength != 256){
        cout <<  ERRMESSAGE << endl;
        return 1;
    }
    
    string keyStr = argv[2];                            //acquire key and parse into unsigned ints
    if (keyStr.compare(0, 2, "0x") == 0){
        keyStr = keyStr.substr(2);
    }
    unsigned int key[keyLength / 32]; //4 bytes/word * 8 bits/byte --> /32
    if((keyStr.length() % 16) != 0 || keyStr.length() > 64 || keyStr.length() < 16){
        cout <<  ERRMESSAGE << endl;
        return 1;
    }
    for (int i = 0; i < (keyLength/32); i++){
        key[i] = stoul(keyStr.substr(hexitPerInt*i,hexitPerInt), 0, 16);
    }

    string nonceStr = argv[3];                          //same for nonce
    unsigned int nonce[2];
    if (nonceStr.compare(0, 2, "0x") == 0){
        nonceStr = nonceStr.substr(2);
    }
    if(nonceStr.length() != 16){
        cout <<  ERRMESSAGE << endl;
        return 1;
    }
    for (int i = 0; i < 2; i++){
        nonce[i] = stoul(nonceStr.substr(hexitPerInt*i,hexitPerInt), 0, 16);
    }

    string textStr = argv[4];                           //same for text
    if (textStr.compare(0, 2, "0x") == 0){
        textStr = textStr.substr(2);
    }
    int textWordCount = (textStr.length() + 7) / hexitPerInt;     //get number of words needed (nibbles/8 nibbles per word)
    int lastWordHexits = textStr.length() % hexitPerInt;          //get number of hexits in the last block (assists with left-side alignment later)
    unsigned int text[textWordCount], cipherText[textWordCount];    //get plaintext and ciphertext space
    for (int i = 0; i < textWordCount; i++){                        //load plaintext
        text[i] = stoul("0x" + textStr.substr(hexitPerInt*i,hexitPerInt), 0, 16);
    }

    unsigned int outputKeystream[16];                           //output keystream
    
    unsigned long blockCount = 0;                               //blockCounter, incremented in salsa208KSG
    int numShift = 4*((hexitPerInt-lastWordHexits)%8);                //(replaced with lastWordHexits) __countl_zero(text[textWordCount-1])-1;      //find number of bits to shift left to align with the key's 8 bits
    text[textWordCount-1] = text[textWordCount-1] << numShift;  //shift

    for(int i = 0; i < textWordCount; i++){                     //XOR text w/ key, key regenerates for every 16 words XORed
        if(i % 16 == 0)
            salsa208KSG(outputKeystream, keyLength, key, nonce, blockCount);        
        cipherText[i] = text[i] ^ outputKeystream[i % 16];
    }
    
    string output;                                         //restructure words into a string
    stringstream ss;
    for(int i = 0; i < textWordCount; i++)
        ss << hex << cipherText[i];
    output = ss.str();
    cout << output.substr(0, output.length()-(numShift/4)) << endl; //truncate any extra text and output
    //*/
}
