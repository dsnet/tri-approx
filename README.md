# Fixed-point Sine and Cosine Approximator #

## Introduction ##

Most embedded hardware (microcontrollers, FPGAs, etc) are too simple to come
with integrated floating-point units. However, sometimes there still arises a
need for complex trigonometric computations. This experimental code attempts to
numerically compute sine and cosine functions for use on FPGAs.

There are many ways to compute sines and cosine, and the most common algorithms
are Taylor series and CORDIC. For this experiment, Taylor series was chosen
over CORDIC because CORDIC uses a relatively large lookup table of constants
to operate. This table occupied more memory than I was willing to sacrifice for
my application.

Since this experiment was targeted towards Altera FPGAs, the bit-width of 18
will appear frequently in the example implementation. This is due to the fact
that Altera hardware multipliers have 18b wide inputs and have 36b wide outputs.
As such the sine and cosine functions will take in a 20b unsigned fixed-point
integer upscaled by 2²⁰ and output a 18b two's complement fixed-point integer
upscaled by 2¹⁷. Notice that the domain of the input is [0,1) and the range of
the output is [-1,+1). This is because the sine and cosine functions implemented
here are normalized such that 0 maps to 0.0 and 2π maps to 1.0. In other words,
the functions being implemented are actually sine(2πx) and cosine(2πx).

## Theory ##

In a fixed-point approximation, it makes sense to make use of the entire input
domain. For that reason, I chose to emulate the normalized functions of
sine(2πx) and cosine(2πx). This way, the entire range of a 20-bit unsigned value
perfectly covers the input domain to sine or cosine for a full period rotation.
The use of normalized functions not only makes better utilization of the input,
but also simplifies the fixed-point arithmetic.

The approximation method used is based on Taylor series, which is a
representation of a function through an infinite sum of terms. The formula for
a Taylor series that represents function *f(x)* centered around point *a* is
the following power series:

![eqn-taylor-series](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-taylor-series.png)

In specific, the type of Taylor series used is technically a Maclaurin series,
since the representation is centered at *a=0*. This means that the power series
converges fastest when *x* is closest to 0. In other words, in a power series of
a finite number of terms, the approximation will be most accurate where *x* is
closest to 0.

Using the formula for a Maclaurin series, the following power series
representations of sine and cosine were derived:

![eqn-sine](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-sine.png)

![eqn-cosine](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-cosine.png)

Since this experiment is approximating trigonometric functions which have many
properties of symmetry and reflection, the power series is only used to estimate
the values of sine or cosine for the region of [0.0,0.25); in the standard
trigonometric functions, this is the region of [0°,90°). The other parts of
trigonometric functions are computed using the first region mirrored in various
ways to exploit the aforementioned properties. This allows the approximated
functions to be significantly more accurate using fewer terms in the series.

With an 18-bit output, it was found that using only 4 non-trivial terms of the
power series approximations was sufficient to achieve an average error that
was less than half the minimum representable range of a the
least-significant-bit (LSB). Adding more terms would increase the amount of
hardware resources needed to approximate the trigonometric functions with
diminishing returns on approximation accuracy.

In both the sine and cosine representations, the values multiplied to the
variable *x* could be pre-computed. The following equation shows computation of
the 8 constants needed for sine (odd indexes) and cosine (even indexes):

![eqn-constants](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-constants.png)

Using only 4 non-trivial terms and the pre-computed constants shown above, the
equations to compute the approximate sine and cosine value is as follows:

![eqn-trig-approx](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-trig-approx.png)

## Results ##

The following Python code demonstrates how the constants were computed and
upscaled to the largest value that fit within an 18-bit unsigned integer.
Upscaling is done since all the arithmetic performed is based on fixed-point
math.

```python
for i in range(1,9):
    val = (2*math.pi)**i / math.factorial(i)
    scale = int(math.log(2**18 / val, 2))
    print "k_%d = %d / 2^%d" % (i, int(round(val*2**scale)), scale)
```

![chart-approx](http://code.digital-static.net/tri-approx/raw/tip/doc/chart-approx.png)

![chart-error](http://code.digital-static.net/tri-approx/raw/tip/doc/chart-error.png)

```
sine
	avg:   0.000003083973
	stdev: 0.000002215256
	max:   0.000013829835
cosine
	avg:   0.000002815008
	stdev: 0.000002052349
	max:   0.000012567624
```

## References ##

* [FPGA Multiplier](http://www.altera.com/literature/hb/cyc2/cyc2_cii51012.pdf) - Embedded multipliers on Cyclone II devices
* [Taylor Series](http://en.wikipedia.org/wiki/Taylor_series) - Wikipedia article on Taylor series
* [CORDIC](http://en.wikipedia.org/wiki/CORDIC) - Wikipedia article on CORDIC
