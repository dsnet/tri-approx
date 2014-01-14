// Written in 2012 by Joe Tsai <joetsai@digital-static.net>
//
// ===================================================================
// The contents of this file are dedicated to the public domain. To
// the extent that dedication to the public domain is not available,
// everyone is granted a worldwide, perpetual, royalty-free,
// non-exclusive license to exercise all rights associated with the
// contents of this file for any purpose whatsoever.
// No rights are reserved.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ===================================================================

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int64_t sine(uint64_t value);
int64_t cosine(uint64_t value);
double fixed_to_float(int64_t value, int scale);
int64_t float_to_fixed(double value, int scale);


int main(int argc, char *argv[]) {
    #if DEMO
        // Simple demo program prints out the 4096 samples of the approximate
        // sine and cosine waves
        printf("sine       cosine  \n");
        int i;
        for (i = 0; i < 4096; i++) {
            int64_t angle = float_to_fixed(1.0/4096.0*i,20);
            int64_t sin_val = sine(angle);
            int64_t cos_val = cosine(angle);
            printf(
                "%+0.6f, %+0.6f\n",
                fixed_to_float(sin_val,17),
                fixed_to_float(cos_val,17)
            );
        }
    #elif SINE
        // Simple sine approximator
        if (argc != 2) {
            printf("Usage: %s ANGLE\n",argv[0]);
            return 1;
        }
        printf(
            "sine(2*PI*%g) = %+0.6f\n",
            atof(argv[1]),
            fixed_to_float(sine(float_to_fixed(atof(argv[1]),20)),17)
        );
    #elif COSINE
        // Simple cosine approximator
        if (argc != 2) {
            printf("Usage: %s ANGLE\n",argv[0]);
            return 1;
        }
        printf(
            "cosine(2*PI*%g) = %+0.6f\n",
            atof(argv[1]),
            fixed_to_float(cosine(float_to_fixed(atof(argv[1]),20)),17)
        );
    #endif

    return 0;
}


// Fixed-point sine approximation. Normalized for an input domain of [0,1)
// instead of the usual domain of [0,2*PI).
//
// Uses Taylor series approximation for sine centered at zero:
//  sine(2*PI*x) = 0 + (2*PI*x)^1/1! - (2*PI*x)^3/3!
//                   + (2*PI*x)^5/5! - (2*PI*x)^7/7!
//               = A*x^1 - B*x^3 + C*x^5 - D*x^7
//
// The bit-width of 18 appears often because it is the width of hardware
// multipliers on Altera FPGAs.
//
// Input: 20-bit unsigned fixed point integer upscaled by 2^20
// Output: 18-bit Two's complement fixed point integer upscaled by 2^17
int64_t sine(uint64_t value) {
    // These are polynomial constants generated for each term in the Taylor
    // series. They have been upscaled to the largest value that fits within
    // 18-bits for greatest precision. The constants labeled with [ADJ] have
    // been manually adjusted to increase accuracy.
    uint64_t A = 205887; // A = round((2*PI)^1/1! * 2^15); Scale: 2^15
    uint64_t B = 169336; // B = round((2*PI)^3/3! * 2^12); Scale: 2^12
    uint64_t C = 167014; // C = round((2*PI)^5/5! * 2^11); Scale: 2^11 [ADJ]
    uint64_t D = 150000; // D = round((2*PI)^7/7! * 2^11); Scale: 2^11 [ADJ]

    // Uses symmetric properties of sine to get more accurate results
    // Normalize the x value to a 18-bit value upscaled by 2^20
    int high0 = ((value >> 19) & 0x01);
    int high1 = ((value >> 18) & 0x01);
    uint64_t x1 = value & 0x3ffff;
    if (high1) {
        x1 = (((uint64_t)0x1 << 18) - x1) & 0x3ffff;
    }
    int negative = high0;
    int one = (!x1 && high1)? 1 : 0;

    // Compute the power values (most of these must be done in series)
    uint64_t x2 = ((x1 * x1) >> 18); // Scale: 2^22
    uint64_t x3 = ((x2 * x1) >> 18); // Scale: 2^24
    uint64_t x5 = ((x2 * x3) >> 18); // Scale: 2^28
    uint64_t x7 = ((x2 * x5) >> 18); // Scale: 2^32

    // Compute the polynomial values (these can be done in parallel)
    uint64_t ax1 = ((A * x1) >> 17); // Scale: 2^18
    uint64_t bx3 = ((B * x3) >> 18); // Scale: 2^18
    uint64_t cx5 = ((C * x5) >> 21); // Scale: 2^18
    uint64_t dx7 = ((D * x7) >> 25); // Scale: 2^18

    // Add all the terms together (these can be done in series-parallel)
    int64_t sum = (ax1 - bx3) + (cx5 - dx7); // Scale: 2^18
    sum = sum >> 1; // Scale: 2^17

    // Perform reflection math and corrections
    if (one) { // Check if sum should be one
        sum = (1 << 17)-1;
    }
    if (negative) { // Check if the sum should be negative
        sum = ~sum + 1;
    } else if (sum & (1 << 17)) { // Check for positive overflow
        sum = (1 << 17)-1;
    }
    return sum;
}


// Fixed-point cosine approximation. Normalized for an input domain of [0,1)
// instead of the usual domain of [0,2*PI).
//
// Uses Taylor series approximation for cosines centered at zero:
//  cosine(2*PI*x) = 1 - (2*PI*x)^2/2! + (2*PI*x)^4/4!
//                     - (2*PI*x)^6/6! + (2*PI*x)^8/8!
//                 = 1 - A*x^2 + B*x^4 - C*x^6 + D*x^8
//
// The bit-width of 18 appears often because it is the width of hardware
// multipliers on Altera FPGAs.
//
// Input: 20-bit unsigned fixed point integer upscaled by 2^20
// Output: 18-bit Two's complement fixed point integer upscaled by 2^17
int64_t cosine(uint64_t value) {
    // These are polynomial constants generated for each term in the Taylor
    // series. They have been upscaled to the largest value that fits within
    // 18-bits for greatest precision. The constants labeled with [ADJ] have
    // been manually adjusted to increase accuracy.
    uint64_t A = 161704; // A = round((2*PI)^2/2! * 2^13); Scale: 2^13
    uint64_t B = 132996; // B = round((2*PI)^4/4! * 2^11); Scale: 2^11
    uint64_t C = 175016; // C = round((2*PI)^6/6! * 2^11); Scale: 2^11
    uint64_t D = 241700; // D = round((2*PI)^8/8! * 2^12); Scale: 2^12 [ADJ]

    // Uses symmetric properties of cosine to get more accurate results
    // Normalize the x value to a 18-bit value upscaled by 2^20
    int high0 = ((value >> 19) & 0x01);
    int high1 = ((value >> 18) & 0x01);
    uint64_t x1 = value & 0x3ffff;
    if (high1) {
        x1 = (((uint64_t)0x1 << 18) - x1) & 0x3ffff;
    }
    int negative = high0 ^ high1;
    int zero = (!x1 && high1)? 1 : 0;

    // Compute the power values (most of these must be done in series)
    uint64_t x2 = ((x1 * x1) >> 18); // Scale: 2^22
    uint64_t x4 = ((x2 * x2) >> 18); // Scale: 2^26
    uint64_t x6 = ((x4 * x2) >> 18); // Scale: 2^30
    uint64_t x8 = ((x4 * x4) >> 18); // Scale: 2^34

    // Compute the polynomial values (these can be done in parallel)
    uint64_t ax2 = ((A * x2) >> 17); // Scale: 2^18
    uint64_t bx4 = ((B * x4) >> 19); // Scale: 2^18
    uint64_t cx6 = ((C * x6) >> 23); // Scale: 2^18
    uint64_t dx8 = ((D * x8) >> 28); // Scale: 2^18

    // Add all the terms together (these can be done in series-parallel)
    int64_t sum = (((int64_t)1 << 18) - ax2) + (bx4 - cx6) + dx8; // Scale: 2^18
    sum = sum >> 1; // Scale: 2^17

    // Perform reflection math and corrections
    if (zero) { // Check if sum should be zero
        sum = 0;
    }
    if (negative) { // Check if the sum should be negative
        sum = ~sum + 1;
    } else if (sum & (1 << 17)) { // Check for positive overflow
        sum = (1 << 17)-1;
    }
    return sum;
}


// Convert a fixed-point value to a floating-point value.
double fixed_to_float(int64_t value, int scale) {
    return (double)value/((int64_t)0x1 << scale);
}


// Convert a floating-point value to a fixed-point value.
int64_t float_to_fixed(double value, int scale) {
    return (int64_t)((value*((int64_t)0x1 << scale)) + 0.5);
}
