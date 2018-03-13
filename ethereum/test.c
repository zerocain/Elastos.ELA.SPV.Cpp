//
//  test
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/27/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <regex.h>
#include "BRCrypto.h"
#include "BRInt.h"
#include "BRKey.h"

#include "BREthereum.h"
#include "../BRBIP39WordsEn.h"
#include "BREthereumLightNode.h"

static void
showHex (uint8_t *source, size_t sourceLen) {
    char *prefix = "{";
    for (int i = 0; i < sourceLen; i++) {
        printf("%s%x", prefix, source[i]);
        prefix = ", ";
    }
    printf ("}\n");
}

//
// Math Tests
//
static void
runMathAddTests () {
  int carry = -1;
  UInt256 z;
  UInt256 x0atMax = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xOne    = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xTwo    = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 x2to32  = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
  UInt256 x7atMax = { .u32 = {          0, 0, 0, 0, 0, 0, 0, UINT32_MAX }};
  UInt256 x7atOne = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
  UInt256 xMax    = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

  z = addUInt256_Overflow (xOne, xOne, &carry);
  assert (2 == z.u32[0] && 0 == carry);

  z = addUInt256_Overflow (xOne, x0atMax, &carry);
  assert (1 == z.u32[1] && 0 == carry);

  z = addUInt256_Overflow (xOne, x2to32, &carry);
  assert (1 == z.u32[0] && 1 == z.u32[1] && 0 == carry);

  z = addUInt256_Overflow (xTwo, x7atOne, &carry);
  assert (2 == z.u32[0] && 1 == z.u32[7] && 0 == carry);
  
  z = addUInt256_Overflow (x7atMax, z, &carry);
  assert (0 == z.u32[7] && 0 == z.u32[0] && 1 == carry);

  z = addUInt256_Overflow (xMax, xOne, &carry);
  assert (1 == carry);
}

static void
runMathSubTests () {
  int negative = -1;
  UInt256 z;
  UInt256 x0atMax = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xOne    = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xOneOne = { .u32 = {          1, 1, 0, 0, 0, 0, 0, 0 }};
  UInt256 xTwo    = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
//  UInt256 x2to32  = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
//  UInt256 x7atMax = { .u32 = {          0, 0, 0, 0, 0, 0, 0, UINT32_MAX }};
//  UInt256 x7atOne = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
//  UInt256 xMax    = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

  z = subUInt256_Negative(xOne, xOne, &negative);
  assert (0 == z.u32[0] && 0 == negative);

  z = subUInt256_Negative(xTwo, xOne, &negative);
  assert (1 == z.u32[0] && 0 == negative);

  z = subUInt256_Negative(xOne, xTwo, &negative);
  assert (1 == z.u32[0] && 1 == negative);

  z = subUInt256_Negative(xOne, x0atMax, &negative);
  assert ((UINT32_MAX - 1) == z.u32[0]
          && 0 == z.u32[7]
          && 1 == negative);

  z = subUInt256_Negative(xOneOne, xTwo, &negative);
  UInt256 zr5 = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
  assert (eqUInt256(zr5, z) && 0 == negative);
  assert (UINT32_MAX == z.u32[0]
          && 0 == z.u32[1]
          && 0 == z.u32[2]
          && 0 == z.u32[7]
          && 0 == negative);
}

static void
runMathMulTests () {
  int carry = -1;
  UInt512 z;
  UInt256 x0atMax   = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xOne      = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 xTwo      = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 x2to31    = { .u32 = {    (1<<31), 0, 0, 0, 0, 0, 0, 0 }};
  UInt256 x2to32    = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
  UInt256 x7atOne   = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
  UInt256 x7at2to31 = { .u32 = {          0, 0, 0, 0, 0, 0, 0, (1<<31) }};
  UInt256 xMax      = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

  //  > (define xMax (- (expt 2 256) 1)
  //  > (number->string xMax 2)
  //  "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"

  z = mulUInt256(xOne, x0atMax);
  assert (UINT32_MAX == z.u32[0] && 0 == z.u32[1] /* && ...*/);

  z = mulUInt256(xOne, x7atOne);
  assert (1 == z.u32[7]);

  z = mulUInt256(x2to31, xTwo);
  assert (0 == z.u32[0] && 1 == z.u32[1] && 0 == z.u32[2] /* && ... */);

  z = mulUInt256(x2to32, x2to32);
  assert (0 == z.u32[0] && 0 == z.u32[1] && 1 == z.u32[2] && 0 == z.u32[3]);

  // (= (* (expt 2 255) (expt 2 255)) (expt 2 510))
  z = mulUInt256(x7at2to31, x7at2to31);
  assert ((1<<30) == z.u32[15]);

  z = mulUInt256(xMax, xMax);
  //  > (number->string (* xMax xMax) 2)
  //  "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111110 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001"
  //  define (factor m)
  //    (if (positive? m)
  //        (begin
  //         (display (remainder m (expt 2 32))) (newline)
  //         (factor (quotient m (expt 2 32))))))
  //  > (factor (* m256 m256))
  //  1
  //  0
  //  0
  //  0
  //  0
  //  0
  //  0
  //  0
  //  4294967294
  //  4294967295
  //  4294967295
  //  4294967295
  //  4294967295
  //  4294967295
  //  4294967295
  //  4294967295
  assert (1 == z.u32[0]
          && 0 == z.u32[1]
          // ...
          && 0 == z.u32[7]
          && UINT32_MAX - 1 == z.u32[ 8]
          && UINT32_MAX == z.u32[ 9]
          && UINT32_MAX == z.u32[10]
          && UINT32_MAX == z.u32[11]
          && UINT32_MAX == z.u32[12]
          && UINT32_MAX == z.u32[13]
          && UINT32_MAX == z.u32[14]
          && UINT32_MAX == z.u32[15]);

}

static void
runMathParseTests () {
  int error;
  UInt256 r = UINT256_ZERO;
  UInt256 a = UINT256_ZERO;

  // "0x09184e72a000" // 10000000000000
  r = createUInt256Parse("09184e72a000", 16, &error);
  a.u64[0] = 10000000000000;
  assert (0 == error && eqUInt256(r, a));

  // "0x0234c8a3397aab58" // 158972490234375000
  r = createUInt256Parse("0234c8a3397aab58", 16, &error);
  a.u64[0] = 158972490234375000;
  assert (0 == error && eqUInt256(r, a));

  // 115792089237316195423570985008687907853269984665640564039457584007913129639935
  r = createUInt256Parse("115792089237316195423570985008687907853269984665640564039457584007913129639935", 10, &error);
  a.u64[0] = a.u64[1] = a.u64[2] = a.u64[3] = UINT64_MAX;
  assert (0 == error && eqUInt256(r, a));

}

static void
runMathTests() {
  runMathParseTests ();
  runMathAddTests();
  runMathSubTests();
  runMathMulTests();
}

//
// Ether Parse
//
static void
runEtherParseTests () {
  BREthereumEtherParseStatus status;
  int overflow;
  BREthereumEther e;
  
  e = etherCreateString("1", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));
  
  e = etherCreateString("100", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

  e = etherCreateString("100.0000", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

  e = etherCreateString("0.001", WEI+1, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

  e = etherCreateString("0.00100", WEI+1, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

  e = etherCreateString("0.001002", ETHER, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1002, ETHER-2)));

  e = etherCreateString("12.03", ETHER, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(12030, ETHER-1)));

  e = etherCreateString("12.03", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_UNDERFLOW == status);

  e = etherCreateString("100000000000000000000000000000000000000000000000000000000000000000000000000000000", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OVERFLOW == status);

  e = etherCreateString("1000000000000000000000", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(1, KETHER)));

  e = etherCreateString("2000000000000000000000.000000", WEI, &status);
  assert (ETHEREUM_ETHER_PARSE_OK == status
          && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(2, KETHER)));
}


/*
m/44'/60'/0'/0/0 :: 0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68
                    0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd
                    0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d
m/44'/60'/0'/0/1 :: 0x9595F373a4eAe74511561A52998cc6fB4F9C2bdD
*/

//
// RLP Test
//
#define RLP_S1 "dog"
#define RLP_S1_RES { 0x83, 'd', 'o', 'g' };

#define RLP_S2 ""
#define RLP_S2_RES { 0x80 }

#define RLP_S3 "Lorem ipsum dolor sit amet, consectetur adipisicing elit"
#define RLP_S3_RES { 0xb8, 0x38, 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', ' ', 'd', 'o', 'l', 'o', 'r', \
 ' ', 's', 'i', 't', ' ', 'a', 'm', 'e', 't', ',', ' ', 'c', 'o', 'n', 's', 'e', 'c', 't', 'e', 't', 'u', 'r', \
 ' ', 'a', 'd', 'i', 'p', 'i', 's', 'i', 'c', 'i', 'n', 'g', ' ', 'e', 'l', 'i', 't' };

#define RLP_V1 0
#define RLP_V1_RES { 0x00 }

#define RLP_V2 15
#define RLP_V2_RES { 0x0f }

#define RLP_V3 1024
#define RLP_V3_RES { 0x82, 0x04, 0x00 }

// 'cat', 'dog'
#define RLP_L1_RES { 0xc8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g' }

int equalBytes (uint8_t *a, size_t aLen, uint8_t *b, size_t bLen) {
    if (aLen != bLen) return 0;
    for (int i = 0; i < aLen; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

void rlpCheck (BRRlpCoder coder, BRRlpItem item, uint8_t *result, size_t resultSize) {
  BRRlpData data;

  rlpGetData(coder, item, &data.bytes, &data.bytesCount);
  assert (equalBytes(data.bytes, data.bytesCount, result, resultSize));
  printf (" => "); showHex (data.bytes, data.bytesCount);

  free (data.bytes);
}

void rlpCheckString (BRRlpCoder coder, const char *string, uint8_t *result, size_t resultSize) {
    printf ("  \"%s\"", string);
    rlpCheck(coder, rlpEncodeItemString(coder, (char*) string), result, resultSize);
}

void rlpCheckInt (BRRlpCoder coder, uint64_t value, uint8_t *result, size_t resultSize) {
    printf ("  %llu", value);
    rlpCheck(coder, rlpEncodeItemUInt64(coder, value), result, resultSize);
}

void runRlpTest () {
    printf ("==== RLP\n");

  BRRlpCoder coder = rlpCoderCreate();

  uint8_t s1r[] = RLP_S1_RES;
  rlpCheckString(coder, RLP_S1, s1r, sizeof(s1r));

  uint8_t s2r[] = RLP_S2_RES;
  rlpCheckString(coder, RLP_S2, s2r, sizeof(s2r));

  uint8_t s3r[] = RLP_S3_RES;
  rlpCheckString(coder, RLP_S3, s3r, sizeof(s3r));

  uint8_t t3r[] = RLP_V1_RES;
  rlpCheckInt(coder, RLP_V1, t3r, sizeof(t3r));

  uint8_t t4r[] = RLP_V2_RES;
  rlpCheckInt(coder, RLP_V2, t4r, sizeof(t4r));

  uint8_t t5r[] = RLP_V3_RES;
  rlpCheckInt(coder, RLP_V3,t5r, sizeof(t5r));

  BRRlpItem listCatDog = rlpEncodeList2(coder,
                                        rlpEncodeItemString(coder, "cat"),
                                        rlpEncodeItemString(coder, "dog"));
  uint8_t resCatDog[] = RLP_L1_RES;
  printf ("  \"%s\"", "[\"cat\" \"dog\"]");
  rlpCheck(coder, listCatDog, resCatDog, 9);

  rlpCoderRelease(coder);
  printf ("\n");
}

//
// Account Test
//
#define TEST_PAPER_KEY    "army van defense carry jealous true garbage claim echo media make crunch"
#define TEST_ETH_ADDR_CHK "0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68"
#define TEST_ETH_ADDR     "0x2161dedc3be05b7bb5aa16154bcbd254e9e9eb68"
// This is a compressed public key; we generate uncompress public keys as { 04 x y }
#define TEST_ETH_PUBKEY   "0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd"
#define TEST_ETH_PRIKEY   "0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d"

void runAddressTests (BREthereumAccount account) {
    BREthereumAddress address = accountGetPrimaryAddress(account);

    printf ("== Address\n");
    printf ("        String: %p\n", address);

    printf ("      PaperKey: %p, %s\n", TEST_PAPER_KEY, TEST_PAPER_KEY);

    const char *publicKeyString = addressPublicKeyAsString (address, 1);
    printf ("    Public Key: %p, %s\n", publicKeyString, publicKeyString);
    assert (0 == strcmp (TEST_ETH_PUBKEY, publicKeyString));

    const char *addressString = addressAsString (address);
    printf ("       Address: %s\n", addressString);
    assert (0 == strcmp (TEST_ETH_ADDR, addressString) ||
	      0 == strcmp (TEST_ETH_ADDR_CHK, addressString));

    free ((void *) addressString);
    free ((void *) publicKeyString);
}

//
// Signature Test
//
// Test signing primitives: BRKeccak256, BRKeyCompactSign. NOTE: there are inconsistencies in the
// Ethereum EIP 155 sources on github.  It does not appear that that 'raw transaction' bytes are
// consistent with the {v, r, s} values 'appended' to the 'unsigned transaction rlp'.  The official
// location is https://github.com/ethereum/EIPs/blob/master/EIPS/eip-155.md - but I've used the
// following (and specifically the 'kvhnuke comment'):
//    https://github.com/ethereum/EIPs/issues/155 (SEE 'kvhnuke commented on Nov 22, 2016')
//
// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
// The "signing data" becomes:
//
// 0xec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080
// The "signing hash" becomes:
//
// 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
//
// If the transaction is signed with the private key
// 0x4646464646464646464646464646464646464646464646464646464646464646, then the v,r,s values become:
//
// (37,
//  11298168949998536842419725113857172427648002808790045841403298480749678639159,
//  26113561835810707062310182368620287328545641189938585203131842552044123671646)
//
//Notice the use of 37 instead of 27. The signed tx would become:
//
// 0xf86c098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a76400008025a028ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83
//

#define SIGNATURE_SIGNING_DATA "ec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080"  // removed "0x"
#define SIGNATURE_SIGNING_HASH "daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53" // removed "0x"
#define SIGNATURE_PRIVATE_KEY  "4646464646464646464646464646464646464646464646464646464646464646"

#define SIGNATURE_V "1b" // 37
#define SIGNATURE_R "28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276" // remove "0x"
#define SIGNATURE_S "67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83" // remove "0x"

#define SIGNING_DATA_2 "f86c258502540be40083035b609482e041e84074fc5f5947d4d27e3c44f824b7a1a187b1a2bc2ec500008078a04a7db627266fa9a4116e3f6b33f5d245db40983234eb356261f36808909d2848a0166fa098a2ce3bda87af6000ed0083e3bf7cc31c6686b670bd85cbc6da2d6e85"
#define SIGNING_HASH_2 "58e5a0fc7fbc849eddc100d44e86276168a8c7baaa5604e44ba6f5eb8ba1b7eb"

void runSignatureTests (BREthereumAccount account) {
    printf ("\n== Signature\n");
    UInt256 digest;

    printf ("    Data 1:\n");
    char *signingData = SIGNATURE_SIGNING_DATA;
    char *signingHash = SIGNATURE_SIGNING_HASH;

    size_t   signingBytesCount = 0;
    uint8_t *signingBytes = decodeHexCreate(&signingBytesCount, signingData, strlen (signingData));

    BRKeccak256(&digest, signingBytes, signingBytesCount);

    char *digestString = encodeHexCreate(NULL, (uint8_t *) &digest, sizeof(UInt256));
    printf ("      Hex: %s\n", digestString);
    assert (0 == strcmp (digestString, signingHash));

    BRKey privateKeyUncompressed;
    BRKeySetPrivKey(&privateKeyUncompressed, SIGNATURE_PRIVATE_KEY);

    size_t signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                           NULL, 0,
                                           digest);

    // Fill the signature
    uint8_t signatureBytes[signatureLen];
    signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                    signatureBytes, signatureLen,
                                    digest);
    assert (65 == signatureLen);

    char *signatureHex = encodeHexCreate(NULL, signatureBytes, signatureLen);
    printf ("      Sig: %s\n", signatureHex);
    assert (130 == strlen(signatureHex));
    assert (0 == strncmp (&signatureHex[ 0], SIGNATURE_V, 2));
    assert (0 == strncmp (&signatureHex[ 2], SIGNATURE_R, 64));
    assert (0 == strncmp (&signatureHex[66], SIGNATURE_S, 64));

    //
    printf ("    Data 2:");
    signingData = SIGNING_DATA_2;
    signingHash = SIGNING_HASH_2;
    signingBytesCount = 0;

    uint8_t *signingBytes2 = decodeHexCreate(&signingBytesCount, signingData, strlen (signingData));

    BRKeccak256(&digest, signingBytes2, signingBytesCount);

    char *digestString2 = encodeHexCreate(NULL, (uint8_t *) &digest, sizeof(UInt256));
    printf ("\n      Hex: %s\n", digestString2);
    assert (0 == strcmp (digestString2, signingHash));
}

//
// Transaction Tests
//
// Take some transactions from 'etherscan.io'; duplicate their content; ensure we process them
// correctly.  Check the


// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
//  The "signing data" becomes:
//     0xec 09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80 01 80 80
//          09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80
//          09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80

//  The "signing hash" becomes:
//     0x2691916f9e6e5b304f135496c08f632040f02d78e36ae5bbbb38f919730c8fa0

#define TEST_TRANS1_NONCE 9
#define TEST_TRANS1_GAS_PRICE_VALUE 20000000000 // 20 GWEI
#define TEST_TRANS1_GAS_PRICE_UNIT  WEI
#define TEST_TRANS1_GAS_LIMIT 21000
#define TEST_TRANS1_TARGET_ADDRESS "0x3535353535353535353535353535353535353535"
#define TEST_TRANS1_ETHER_AMOUNT 1000000000000000000u // 1 ETHER
#define TEST_TRANS1_ETHER_AMOUNT_UNIT WEI
#define TEST_TRANS1_DATA ""

#define TEST_TRANS1_RESULT "ec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080"

void runTransactionTests1 (BREthereumAccount account, BREthereumNetwork network) {
    BREthereumWallet  wallet = walletCreate(account, network);

    BREthereumTransaction transaction = walletCreateTransactionDetailed(
            wallet,
            createAddress(TEST_TRANS1_TARGET_ADDRESS),
            holdingCreateEther(etherCreateNumber(TEST_TRANS1_ETHER_AMOUNT, TEST_TRANS1_ETHER_AMOUNT_UNIT)),
            gasPriceCreate(etherCreateNumber(TEST_TRANS1_GAS_PRICE_VALUE, TEST_TRANS1_GAS_PRICE_UNIT)),
            gasCreate(TEST_TRANS1_GAS_LIMIT),
            TEST_TRANS1_NONCE);

    assert (1 == networkGetChainId(network));
    BRRlpData dataUnsignedTransaction = transactionEncodeRLP(transaction, network, TRANSACTION_RLP_UNSIGNED);

    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("  Tx1 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS1_RESULT));
}

// https://etherscan.io/tx/0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// Hash: 0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// From: 0x23c2a202c38331b91980a8a23d31f4ca3d0ecc2b
//   to: 0x873feb0644a6fbb9532bb31d1c03d4538aadec30
// Amnt: 0.5 Ether ($429.90)
// GasL: 21000
// GasP: 2 Gwei
// Nonc: 1
// Data: 0x
//
//  Raw: 0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
//       List  Nonc  GasP      GasL          RecvAddr                               Amount         Data   <signature>
// Rslt:        01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80

#define TEST_TRANS2_NONCE 1
#define TEST_TRANS2_GAS_PRICE_VALUE 2000000000 // 20 GWEI
#define TEST_TRANS2_GAS_PRICE_UNIT  WEI
#define TEST_TRANS2_GAS_LIMIT 21000
#define TEST_TRANS2_TARGET_ADDRESS "0x873feb0644a6fbb9532bb31d1c03d4538aadec30"
#define TEST_TRANS2_ETHER_AMOUNT 500000000000000000u // 0.5 ETHER
#define TEST_TRANS2_ETHER_AMOUNT_UNIT WEI
#define TEST_TRANS2_DATA ""

#define TEST_TRANS2_RESULT_SIGNED   "f86b01847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b200008026a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde"
#define TEST_TRANS2_RESULT_UNSIGNED   "eb01847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b2000080018080"

void runTransactionTests2 (BREthereumAccount account, BREthereumNetwork network) {

    BREthereumWallet  wallet = walletCreate(account, network);

    BREthereumTransaction transaction = walletCreateTransactionDetailed(
            wallet,
            createAddress(TEST_TRANS2_TARGET_ADDRESS),
            holdingCreateEther(etherCreateNumber(TEST_TRANS2_ETHER_AMOUNT, TEST_TRANS2_ETHER_AMOUNT_UNIT)),
            gasPriceCreate(etherCreateNumber(TEST_TRANS2_GAS_PRICE_VALUE, TEST_TRANS2_GAS_PRICE_UNIT)),
            gasCreate(TEST_TRANS2_GAS_LIMIT),
            TEST_TRANS2_NONCE);

    assert (1 == networkGetChainId(network));
    BRRlpData dataUnsignedTransaction = transactionEncodeRLP(transaction, network, TRANSACTION_RLP_UNSIGNED);

    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("  Tx2 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS2_RESULT_UNSIGNED));
}

void runTransactionTests (BREthereumAccount account, BREthereumNetwork network) {
    printf ("\n== Transaction\n");

    runTransactionTests1 (account, network);
    runTransactionTests2 (account, network);
}

//
// Account Tests
//
void runAccountTests () {

    BREthereumAccount account = createAccount(TEST_PAPER_KEY);
    BREthereumNetwork network = ethereumMainnet;

    printf ("==== Account: %p\n", account);
    runAddressTests(account);
    runSignatureTests(account);
    runTransactionTests(account, network);

    accountFree (account);
    printf ("\n\n");
}

//
// Light Node Tests
//
#define NODE_PAPER_KEY "ginger settle marine tissue robot crane night number ramp coast roast critic"
#define NODE_NONCE              TEST_TRANS2_NONCE // 1
#define NODE_GAS_PRICE_VALUE    TEST_TRANS2_GAS_PRICE_VALUE // 20 GWEI
#define NODE_GAS_PRICE_UNIT     TEST_TRANS2_GAS_PRICE_UNIT // WEI
#define NODE_GAS_LIMIT          TEST_TRANS2_GAS_LIMIT
#define NODE_RECV_ADDR          TEST_TRANS2_TARGET_ADDRESS
#define NODE_ETHER_AMOUNT_UNIT  TEST_TRANS2_ETHER_AMOUNT_UNIT
#define NODE_ETHER_AMOUNT       TEST_TRANS2_ETHER_AMOUNT

#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000

#define GAS_LIMIT_DEFAULT 21000

//  Result      f86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a053ee5877032551f52da516c83620273312c8ab5a773d482dd60a772bb4a39938a07e187ee2335bfcfa3d20119e0e424d9ef5a81452dadee91ef2daf40081fdc454
//  Raw:      0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26,a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
#define NODE_RESULT "01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a0594c2fe40942a9dbd75b9cdd09397016592fc98ae24226f41706c5004c6608d0a072861c46ae62f4aae06eba04e5708b9421d2fcf21fa7f02aed1ff04accd405e3"

void prepareTransaction (const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {

  // START - One Time Code Block
  BREthereumLightNodeConfiguration configuration =
    lightNodeConfigurationCreateLES(ethereumMainnet, 0);
  BREthereumLightNode node = createLightNode(configuration);
  BREthereumLightNodeAccountId account = lightNodeCreateAccount(node, paperKey);
  BREthereumNetwork network = ethereumMainnet;
  // A wallet holding Ether
  BREthereumLightNodeWalletId wallet = lightNodeCreateWallet(node, account, network);
  // END - One Time Code Block

  // Optional - will provide listNodeWalletCreateTransactionDetailed.
  lightNodeSetWalletGasPrice(node, wallet, WEI, gasPrice);
  lightNodeSetWalletGasLimit(node, wallet, gasLimit);

  BREthereumEther amountInEther =
    lightNodeCreateEtherAmountUnit(node, amount, WEI);

  BREthereumLightNodeTransactionId tx1 =
    lightNodeWalletCreateTransaction
      (node,
       wallet,
       recvAddr,
       amountInEther);

  lightNodeWalletSignTransaction (node, wallet, tx1, paperKey);

  const char *rawTransactionHexEncoded =
    lightNodeGetTransactionRawDataHexEncoded(node, wallet, tx1, "Ox");

  printf ("        Raw Transaction: %s\n", rawTransactionHexEncoded);
}

void runLightNodeTests () {
  printf ("==== Light Node\n");
  prepareTransaction(NODE_PAPER_KEY, NODE_RECV_ADDR, TEST_TRANS2_GAS_PRICE_VALUE, GAS_LIMIT_DEFAULT, NODE_ETHER_AMOUNT);
}

// Local (PaperKey) -> LocalTest @ 5 GWEI gasPrice @ 21000 gasLimit & 0.0001/2 ETH
#define ACTUAL_RAW_TX "f86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260"
// eth.sendRawTran ('0xf86a01841dcd65008252089422583f6c7dae5032f4d72a10b9e9fa977cbfc5f68701c6bf52634000801ca05d27cbd6a84e5d34bb20ce7dade4a21efb4da7507958c17d7f92cfa99a4a9eb6a005fcb9a61e729b3c6b0af3bad307ef06cdf5c5578615fedcc4163a2aa2812260', function (err, hash) { if (!err) console.log(hash); });
extern void
reallySend () {
  char paperKey[1024];
  char recvAddress[1024];

  fputs("PaperKey: ", stdout);
  fgets (paperKey, 1024, stdin);
  paperKey[strlen(paperKey) - 1] = '\0';

  fputs("Address: ", stdout);
  fgets (recvAddress, 1024, stdin);
  recvAddress[strlen(recvAddress) - 1] = '\0';

  printf ("PaperKey: '%s'\nAddress: '%s'\n", paperKey, recvAddress);

  // 0.001/2 ETH
  prepareTransaction(paperKey, recvAddress, GAS_PRICE_5_GWEI, GAS_LIMIT_DEFAULT, 1000000000000000000 / 1000 / 2);
}
//
// All Tests
//

extern void
runTests (void) {
  installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);\
  runMathTests();
  runEtherParseTests();
  runRlpTest();
  runAccountTests();
  runLightNodeTests();
  //    reallySend();
}

int main(int argc, const char *argv[]) {
    runTests();
}

