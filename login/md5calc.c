/***********************************************************
 * md5�v�Z�A���S���Y��
 *
 * �\�[�X�R�[�h�͈ȉ���URL���Q�l�ɂ��܂���
 * http://www.geocities.co.jp/SiliconValley-Oakland/8878/lab17/lab17.html
 *
 ***********************************************************/

#include "md5calc.h"
#include <string.h>
#include <stdio.h>

#ifndef UINT_MAX
#define UINT_MAX 4294967295U
#endif

// �O���[�o���ϐ�
static unsigned int *pX;

// Stirng Table
static const unsigned int T[] = {
   0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, //0
   0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, //4
   0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, //8
   0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, //12
   0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, //16
   0xd62f105d,  0x2441453, 0xd8a1e681, 0xe7d3fbc8, //20
   0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, //24
   0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, //28
   0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, //32
   0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, //36
   0x289b7ec6, 0xeaa127fa, 0xd4ef3085,  0x4881d05, //40
   0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, //44
   0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, //48
   0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, //52
   0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, //56
   0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391  //60
};

// ROTATE_LEFT�� x ������n�r�b�g��]������B�����RFC���炻�̂܂ܗ��p
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// ���̑��̌v�Z�Ɏg���֐�
static unsigned int F(unsigned int X, unsigned int Y, unsigned int Z)
{
   return (X & Y) | (~X & Z);
}
static unsigned int G(unsigned int X, unsigned int Y, unsigned int Z)
{
   return (X & Z) | (Y & ~Z);
}
static unsigned int H(unsigned int X, unsigned int Y, unsigned int Z)
{
   return X ^ Y ^ Z;
}
static unsigned int I(unsigned int X, unsigned int Y, unsigned int Z)
{
   return Y ^ (X | ~Z);
}

static unsigned int Round(unsigned int a, unsigned int b, unsigned int FGHI,
                     unsigned int k, unsigned int s, unsigned int i)
{
   return b + ROTATE_LEFT(a + FGHI + pX[k] + T[i], s);
}

static void Round1(unsigned int *a, unsigned int b, unsigned int c,
		unsigned int d,unsigned int k, unsigned int s, unsigned int i)
{
	*a = Round(*a, b, F(b,c,d), k, s, i);
}
static void Round2(unsigned int *a, unsigned int b, unsigned int c,
		unsigned int d,unsigned int k, unsigned int s, unsigned int i)
{
	*a = Round(*a, b, G(b,c,d), k, s, i);
}
static void Round3(unsigned int *a, unsigned int b, unsigned int c,
		unsigned int d,unsigned int k, unsigned int s, unsigned int i)
{
	*a = Round(*a, b, H(b,c,d), k, s, i);
}
static void Round4(unsigned int *a, unsigned int b, unsigned int c,
		unsigned int d,unsigned int k, unsigned int s, unsigned int i)
{
	*a = Round(*a, b, I(b,c,d), k, s, i);
}

static void MD5_Round_Calculate(const unsigned char *block,
	unsigned int *A2, unsigned int *B2, unsigned int *C2, unsigned int *D2)
{
	//create X �K�v�Ȃ̂�
	unsigned int X[16]; //512bit 64byte
	int j,k;

	//Save A as AA, B as BB, C as CC, and D as DD (A,B,C,D�̕ۑ�)
	unsigned int A=*A2, B=*B2, C=*C2, D=*D2;
	unsigned int AA = A,BB = B,CC = C,DD = D;
	
	//���E���h�̌v�Z�ׂ̈Ɏd���Ȃ����ϐ����B�B�B for Round1...4
	pX = X;

	//Copy block(padding_message) i into X
	for (j=0,k=0; j<64; j+=4,k++)
		X[k] = ( (unsigned int )block[j] )         // 8byte*4 -> 32byte �ϊ�
			| ( ((unsigned int )block[j+1]) << 8 ) // RFC�ł���Decode�Ƃ����֐�
			| ( ((unsigned int )block[j+2]) << 16 )
			| ( ((unsigned int )block[j+3]) << 24 );


   //Round 1
   Round1(&A,B,C,D,  0, 7,  0); Round1(&D,A,B,C,  1, 12,  1); Round1(&C,D,A,B,  2, 17,  2); Round1(&B,C,D,A,  3, 22,  3);
   Round1(&A,B,C,D,  4, 7,  4); Round1(&D,A,B,C,  5, 12,  5); Round1(&C,D,A,B,  6, 17,  6); Round1(&B,C,D,A,  7, 22,  7);
   Round1(&A,B,C,D,  8, 7,  8); Round1(&D,A,B,C,  9, 12,  9); Round1(&C,D,A,B, 10, 17, 10); Round1(&B,C,D,A, 11, 22, 11);
   Round1(&A,B,C,D, 12, 7, 12); Round1(&D,A,B,C, 13, 12, 13); Round1(&C,D,A,B, 14, 17, 14); Round1(&B,C,D,A, 15, 22, 15);

   //Round 2
   Round2(&A,B,C,D,  1, 5, 16); Round2(&D,A,B,C,  6, 9, 17); Round2(&C,D,A,B, 11, 14, 18); Round2(&B,C,D,A,  0, 20, 19);
   Round2(&A,B,C,D,  5, 5, 20); Round2(&D,A,B,C, 10, 9, 21); Round2(&C,D,A,B, 15, 14, 22); Round2(&B,C,D,A,  4, 20, 23);
   Round2(&A,B,C,D,  9, 5, 24); Round2(&D,A,B,C, 14, 9, 25); Round2(&C,D,A,B,  3, 14, 26); Round2(&B,C,D,A,  8, 20, 27);
   Round2(&A,B,C,D, 13, 5, 28); Round2(&D,A,B,C,  2, 9, 29); Round2(&C,D,A,B,  7, 14, 30); Round2(&B,C,D,A, 12, 20, 31);

   //Round 3
   Round3(&A,B,C,D,  5, 4, 32); Round3(&D,A,B,C,  8, 11, 33); Round3(&C,D,A,B, 11, 16, 34); Round3(&B,C,D,A, 14, 23, 35);
   Round3(&A,B,C,D,  1, 4, 36); Round3(&D,A,B,C,  4, 11, 37); Round3(&C,D,A,B,  7, 16, 38); Round3(&B,C,D,A, 10, 23, 39);
   Round3(&A,B,C,D, 13, 4, 40); Round3(&D,A,B,C,  0, 11, 41); Round3(&C,D,A,B,  3, 16, 42); Round3(&B,C,D,A,  6, 23, 43);
   Round3(&A,B,C,D,  9, 4, 44); Round3(&D,A,B,C, 12, 11, 45); Round3(&C,D,A,B, 15, 16, 46); Round3(&B,C,D,A,  2, 23, 47);

   //Round 4
   Round4(&A,B,C,D,  0, 6, 48); Round4(&D,A,B,C,  7, 10, 49); Round4(&C,D,A,B, 14, 15, 50); Round4(&B,C,D,A,  5, 21, 51);
   Round4(&A,B,C,D, 12, 6, 52); Round4(&D,A,B,C,  3, 10, 53); Round4(&C,D,A,B, 10, 15, 54); Round4(&B,C,D,A,  1, 21, 55);
   Round4(&A,B,C,D,  8, 6, 56); Round4(&D,A,B,C, 15, 10, 57); Round4(&C,D,A,B,  6, 15, 58); Round4(&B,C,D,A, 13, 21, 59);
   Round4(&A,B,C,D,  4, 6, 60); Round4(&D,A,B,C, 11, 10, 61); Round4(&C,D,A,B,  2, 15, 62); Round4(&B,C,D,A,  9, 21, 63);

   // Then perform the following additions. (���Z���܂��傤)
   *A2 = A + AA;
   *B2 = B + BB;
   *C2 = C + CC;
   *D2 = D + DD;

   //�@�����̃N���A
   memset(pX, 0, sizeof(X));
}

//-------------------------------------------------------------------
// �O���p�֐�

/** string�͕������������������output�͕��������ꂽ�o�C�i�� */
void MD5_String2binary(const char * string, char * output)
{
//var
   /*8bit*/
   unsigned char padding_message[64]; //�g�����b�Z�[�W 512bit 64byte
   unsigned char *pstring;             //���ݑ���������string�̈ʒu��ێ�
//   unsigned char digest[16];
   /*32bit*/
   unsigned int string_byte_len,    //string�̃o�C�g����ێ�
                   string_bit_len,     //string�̃r�b�g����ێ�
                   copy_len,           //1-3�Ŏg���c�����o�C�g��
                   msg_digest[4];      //���b�Z�[�W�_�C�W�F�X�g 128bit 4byte
   unsigned int *A = &msg_digest[0], //RFC�ɑ��������b�Z�[�W�_�C�W�F�X�g�i�̃��t�@�����X�j
                   *B = &msg_digest[1],
                   *C = &msg_digest[2],
                   *D = &msg_digest[3];
	int i;

//prog
   //Step 3. Initialize MD Buffer (A,B,C,D�̏�����;�X�e�b�v�R�ł����d���Ȃ��擪��)
   *A = 0x67452301;
   *B = 0xefcdab89;
   *C = 0x98badcfe;
   *D = 0x10325476;

   //Step 1. Append Padding Bits (�����r�b�g�̊g��)
   //1-1
   string_byte_len = strlen(string);    //������̃o�C�g�����擾
   pstring = (unsigned char *)string; //���݂̕�����̈ʒu���Z�b�g

   //1-2 �������U�S�o�C�g�����ɂȂ�܂Ōv�Z���J��Ԃ�
   for (i=string_byte_len; 64<=i; i-=64,pstring+=64)
        MD5_Round_Calculate(pstring, A,B,C,D);

   //1-3
   copy_len = string_byte_len % 64;                               //�c�����o�C�g�����Z�o
   strncpy((char *)padding_message, (char *)pstring, copy_len); //�g���r�b�g��փ��b�Z�[�W���R�s�[
   memset(padding_message+copy_len, 0, 64 - copy_len);           //�g���r�b�g���ɂȂ�܂�0�Ŗ��߂�
   padding_message[copy_len] |= 0x80;                            //���b�Z�[�W�̎���1

   //1-4 
   //�c�肪56�o�C�g�ȏ�i�U�S�o�C�g�����j�Ȃ�΂U�S�o�C�g�Ɋg�����Čv�Z
   if (56 <= copy_len) {
       MD5_Round_Calculate(padding_message, A,B,C,D);

       memset(padding_message, 0, 56); //�V���ɂT�U�o�C�g���O�Ŗ��߂�
   }


   //Step 2. Append Length (�����̏���ǉ�)
   string_bit_len = string_byte_len * 8;             //�o�C�g������r�b�g���ցi���ʂR�Q�o�C�g�j
   memcpy(&padding_message[56], &string_bit_len, 4); //���ʂR�Q�o�C�g���Z�b�g

   //���ʂR�Q�o�C�g�����ł̓r�b�g����\���ł��Ȃ��Ƃ��͏�ʂɌ��グ
  if (UINT_MAX / 8 < string_byte_len) {
      unsigned int high = (string_byte_len - UINT_MAX / 8) * 8;
      memcpy(&padding_message[60], &high, 4);
  } else
      memset(&padding_message[60], 0, 4); //���̏ꍇ�͏�ʂɂ͂O�ł悢

   //Step 4. Process Message in 16-Word Blocks (MD5�̌v�Z)
   MD5_Round_Calculate(padding_message, A,B,C,D);


   //Step 5. Output (�o��)
   memcpy(output,msg_digest,16);
//   memcpy(digest, msg_digest, 16); //8byte*4 <- 32byte �ϊ� RFC�ł���Encode�Ƃ����֐�
/*   sprintf(output,
           "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
           digest[ 0], digest[ 1], digest[ 2], digest[ 3],
           digest[ 4], digest[ 5], digest[ 6], digest[ 7],
           digest[ 8], digest[ 9], digest[10], digest[11],
           digest[12], digest[13], digest[14], digest[15]);*/
}

/** string�͕������������������output�͕��������ꂽ������ */
void MD5_String(const char * string, char * output)
{
   unsigned char digest[16];

	MD5_String2binary(string,digest);
	sprintf(output,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		digest[ 0], digest[ 1], digest[ 2], digest[ 3],
		digest[ 4], digest[ 5], digest[ 6], digest[ 7],
		digest[ 8], digest[ 9], digest[10], digest[11],
		digest[12], digest[13], digest[14], digest[15]);
}

