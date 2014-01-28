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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


int64_t sine(uint64_t value);
int64_t cosine(uint64_t value);
double fixed_to_float(int64_t value, int scale);
int64_t float_to_fixed(double value, int scale);
int64_t clamp_overflow(int64_t value, int width);


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
    #elif STATS
        // Compute the estimation error across all possible values
        size_t i;
        int domain = 1 << 20;
        double fdomain = (double)domain;

        double* sin_errs = calloc(domain, sizeof(double));
        double* cos_errs = calloc(domain, sizeof(double));
        for (i = 0; i < domain; i++) {
            // Compute the real value from C math library
            double float_angle = 2*M_PI*1.0/fdomain*i;
            double float_sin = sin(float_angle);
            double float_cos = cos(float_angle);

            // Compute the estimated value fixed-point methods library
            int64_t fixed_angle = float_to_fixed(1.0/fdomain*i,20);
            double fixed_sin = fixed_to_float(sine(fixed_angle),17);
            double fixed_cos = fixed_to_float(cosine(fixed_angle),17);

            sin_errs[i] = fabs(float_sin - fixed_sin);
            cos_errs[i] = fabs(float_cos - fixed_cos);
        }

        // Compute average
        double sin_err_sum = 0.0;
        double cos_err_sum = 0.0;
        for (i = 0; i < domain; i++) {
            sin_err_sum += sin_errs[i];
            cos_err_sum += cos_errs[i];
        }
        double sin_err_avg = sin_err_sum / fdomain;
        double cos_err_avg = cos_err_sum / fdomain;

        // Compute standard deviation
        double sin_err_sumsqr = 0.0;
        double cos_err_sumsqr = 0.0;
        for (i = 0; i < domain; i++) {
            sin_err_sumsqr += pow(sin_errs[i] - sin_err_avg, 2.0);
            cos_err_sumsqr += pow(cos_errs[i] - cos_err_avg, 2.0);
        }
        double sin_err_stdev = sqrt(sin_err_sumsqr / fdomain);
        double cos_err_stdev = sqrt(cos_err_sumsqr / fdomain);

        // Compute the maximum error
        double sin_err_max = 0.0;
        double cos_err_max = 0.0;
        for (i = 0; i < domain; i++) {
            sin_err_max = fmax(sin_err_max, sin_errs[i]);
            cos_err_max = fmax(cos_err_max, cos_errs[i]);
        }

        printf("sine\n");
        printf("\tavg:   %0.12f\n", sin_err_avg);
        printf("\tstdev: %0.12f\n", sin_err_stdev);
        printf("\tmax:   %0.12f\n", sin_err_max);
        printf("cosine\n");
        printf("\tavg:   %0.12f\n", cos_err_avg);
        printf("\tstdev: %0.12f\n", cos_err_stdev);
        printf("\tmax:   %0.12f\n", cos_err_max);

        free(sin_errs);
        free(cos_errs);
    #endif

    return 0;
}


// Fixed-point sine approximation. Normalized for an input domain of [0,1)
// instead of the usual domain of [0,2*PI).
//
// Uses Taylor series approximation for sine centered at zero:
//  sine(2*PI*x) = 0 + (2*PI*x)^1/1! - (2*PI*x)^3/3!
//                   + (2*PI*x)^5/5! - (2*PI*x)^7/7!
//               = k_1*x^1 - k_3*x^3 + k_5*x^5 - k_7*x^7
//
// The bit-width of 18 appears often because it is the width of hardware
// multipliers on Altera FPGAs.
//
// Input: 20-bit unsigned fixed point integer upscaled by 2^20
// Output: 18-bit two's complement fixed point integer upscaled by 2^17
int64_t sine(uint64_t value) {
    // These are polynomial constants generated for each term in the Taylor
    // series. They have been upscaled to the largest value that fits within
    // 18-bits for greatest precision. The constants labeled with [ADJ] have
    // been manually adjusted to increase accuracy.
    const uint64_t k1 = 205887; // k1 = round((2*PI)^1/1! * 2^15)
    const uint64_t k3 = 169336; // k3 = round((2*PI)^3/3! * 2^12)
    const uint64_t k5 = 167014; // k5 = round((2*PI)^5/5! * 2^11) [ADJ]
    const uint64_t k7 = 150000; // k7 = round((2*PI)^7/7! * 2^11) [ADJ]

    // Uses symmetric properties of sine to get more accurate results
    // Normalize the x value to a 18-bit value upscaled by 2^20
    bool high0 = ((value >> 19) & 0x01);
    bool high1 = ((value >> 18) & 0x01);
    uint64_t x1 = value & 0x3ffff; // Strip two highest bits
    if (high1) {
        x1 = (((uint64_t)0x1 << 18) - x1) & 0x3ffff;
    }
    bool negative = high0;
    bool one = (x1 == 0) && high1;

    // Compute the power values (most of these must be done in series)
    uint64_t x2 = ((x1 * x1) >> 18); // Scale: 2^22
    uint64_t x3 = ((x2 * x1) >> 18); // Scale: 2^24
    uint64_t x5 = ((x2 * x3) >> 18); // Scale: 2^28
    uint64_t x7 = ((x2 * x5) >> 18); // Scale: 2^32

    // Compute the polynomial values (these can be done in parallel)
    uint64_t kx1 = ((k1 * x1) >> 17); // Scale: 2^18
    uint64_t kx3 = ((k3 * x3) >> 18); // Scale: 2^18
    uint64_t kx5 = ((k5 * x5) >> 21); // Scale: 2^18
    uint64_t kx7 = ((k7 * x7) >> 25); // Scale: 2^18

    // Add all the terms together (these can be done in series-parallel)
    int64_t sum = kx1 - kx3 + kx5 - kx7; // Scale: 2^18
    sum = sum >> 1; // Scale: 2^17

    // Perform reflection math and corrections
    if (one) { // Check if sum should be one
        sum = (1 << 17);
    }
    if (negative) { // Check if the sum should be negative
        sum = ~sum + 1;
    }
    return clamp_overflow(sum, 18);
}


// Fixed-point cosine approximation. Normalized for an input domain of [0,1)
// instead of the usual domain of [0,2*PI).
//
// Uses Taylor series approximation for cosines centered at zero:
//  cosine(2*PI*x) = 1 - (2*PI*x)^2/2! + (2*PI*x)^4/4!
//                     - (2*PI*x)^6/6! + (2*PI*x)^8/8!
//                 = 1 - k_2*x^2 + k_4*x^4 - k_6*x^6 + k_8*x^8
//
// The bit-width of 18 appears often because it is the width of hardware
// multipliers on Altera FPGAs.
//
// Input: 20-bit unsigned fixed point integer upscaled by 2^20
// Output: 18-bit two's complement fixed point integer upscaled by 2^17
int64_t cosine(uint64_t value) {
    // These are polynomial constants generated for each term in the Taylor
    // series. They have been upscaled to the largest value that fits within
    // 18-bits for greatest precision. The constants labeled with [ADJ] have
    // been manually adjusted to increase accuracy.
    const uint64_t k2 = 161704; // k2 = round((2*PI)^2/2! * 2^13)
    const uint64_t k4 = 132996; // k4 = round((2*PI)^4/4! * 2^11)
    const uint64_t k6 = 175016; // k6 = round((2*PI)^6/6! * 2^11)
    const uint64_t k8 = 241700; // k8 = round((2*PI)^8/8! * 2^12) [ADJ]

    // Uses symmetric properties of cosine to get more accurate results
    // Normalize the x value to a 18-bit value upscaled by 2^20
    bool high0 = ((value >> 19) & 0x01);
    bool high1 = ((value >> 18) & 0x01);
    uint64_t x1 = value & 0x3ffff; // Strip two highest bits
    if (high1) {
        x1 = (((uint64_t)0x1 << 18) - x1) & 0x3ffff;
    }
    bool negative = high0 ^ high1;
    bool zero = (x1 == 0) && high1;

    // Compute the power values (most of these must be done in series)
    uint64_t x2 = ((x1 * x1) >> 18); // Scale: 2^22
    uint64_t x4 = ((x2 * x2) >> 18); // Scale: 2^26
    uint64_t x6 = ((x4 * x2) >> 18); // Scale: 2^30
    uint64_t x8 = ((x4 * x4) >> 18); // Scale: 2^34

    // Compute the polynomial values (these can be done in parallel)
    uint64_t kx2 = ((k2 * x2) >> 17); // Scale: 2^18
    uint64_t kx4 = ((k4 * x4) >> 19); // Scale: 2^18
    uint64_t kx6 = ((k6 * x6) >> 23); // Scale: 2^18
    uint64_t kx8 = ((k8 * x8) >> 28); // Scale: 2^18

    // Add all the terms together (these can be done in series-parallel)
    int64_t sum = ((int64_t)1 << 18) - kx2 + kx4 - kx6 + kx8; // Scale: 2^18
    sum = sum >> 1; // Scale: 2^17

    // Perform reflection math and corrections
    if (zero) { // Check if sum should be zero
        sum = 0;
    }
    if (negative) { // Check if the sum should be negative
        sum = ~sum + 1;
    }
    return clamp_overflow(sum, 18);
}


// Convert a fixed-point value to a floating-point value.
double fixed_to_float(int64_t value, int scale) {
    return (double)value/((int64_t)0x1 << scale);
}


// Convert a floating-point value to a fixed-point value.
int64_t float_to_fixed(double value, int scale) {
    return (int64_t)((value*((int64_t)0x1 << scale)) + 0.5);
}


// Clamp an overflowed fixed-point two's complement value.
int64_t clamp_overflow(int64_t value, int width) {
    bool high0 = ((value >> (width+0)) & 0x01);
    bool high1 = ((value >> (width-1)) & 0x01);
    if (high0 ^ high1) {
        if (high0) {
            value = (-1 << width-1);
        } else {
            value = (1 << width-1) - 1;
        }
    }

    // Sanity check to ensure there is no overflow
    int64_t high = value >> width-1;
    assert((high == 0x0000000000000000LL) || (high == 0xFFFFFFFFFFFFFFFFLL));
    return value;
}
