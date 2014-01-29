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


## Implementation ##

In order to reduce the amount of hardware resources needed, the *k* constants
defined above were pre-computed. The following Python code demonstrates how the
constants were computed and upscaled to the largest value that fit within an
18-bit unsigned integer. Upscaling is done since all the arithmetic performed
is based on fixed-point math. The constants actually used in the example
implementation are not exactly the ones generated by the mini-script since some
constants were manually tweaked for accuracy.

```python
for i in range(1,9):
    val = (2*math.pi)**i / math.factorial(i)
    scale = int(math.log(2**18 / val, 2))
    print "k%d = %d / 2^%d" % (i, int(round(val*2**scale)), scale)
```

Since the target platform was an FPGA, the operations involved with Taylor
series approximation allow for the designs to be easily pipelined.
It was assumed that the hardware multipliers held the highest latency and would
take a single cycle to execute, while all addition and subtraction operations
together could complete in a single cycle.

![pipeline-sine](http://code.digital-static.net/tri-approx/raw/tip/doc/pipeline-sine_lite.png)

Pipelined designs of sine. Sine requires more registers to hold state between
stages and thus uses more hardware resources than cosine.
Furthermore, analysis later will show that sine is also less accurate.

![pipeline-cosine](http://code.digital-static.net/tri-approx/raw/tip/doc/pipeline-cosine_lite.png)

Pipelined designs of cosine. Note that the stage to compute *x⁸* could be
reduced since it could be computed in parallel with *x⁶* by squaring *x⁴*.
This extra stage was kept so that the pipeline lengths would be identical for
sine and cosine computations.

In both FPGA designs, the pipeline length is 6 stages. The shaded green regions
represent logic needed to do reflections and corrections, while the shaded blue
regions represent the logic actually needed to do the Taylor series expansions.
Also, all the constants shown are not upscaled according to their fixed-point
counterpoints. In general, the bit-widths of the data lines is 18b.
However, the wiring to shift the bits is not shown.

The *HIGH0* and *HIGH1* operators at the start of the pipelines extract the
most-significant-bit and second most-significant-bit, respectively. The *STRIP*
operator removes the top two bits. The *CLAMP* operator at the end of both
pipelines is performing the overflow check as shown in the C implementation.


## Results ##

Using the values generated from the C implementation, we can plot the sine
and cosine functions approximated by this method. In the graph below, it appears
that only a single sine and a single cosine is graphed. In actuality, the real,
floating-point approximate, and fixed-point approximate are graphed together.
It is clear from a graph of this resolution that their errors are negligible.
The floating-point approximate is the result obtained when the 4-term equations
listed above are computed using IEEE 754 floating-point units, while the
fixed-point approximate is the result obtained using the 18b wide fixed-point
arithmetic in the C implementation.

![chart-approx](http://code.digital-static.net/tri-approx/raw/tip/doc/chart-approx.png)

With 18-bits used for the fixed-point representation, the LSB maps to a quantum
of about 7.63E-6. If we could approximate sines and cosines perfectly, we would
expect an maximum errors to be no worse than half the quantum. We will define
this value of 3.81E-6 as the error epsilon, *ε*.

However, since we are only using a 4-term approximation, we obviously cannot get
maximum errors below *ε*. For this reason, some of the *k* constants in the later
terms were manually adjusted to compensate for the lack of infinite terms at the
end. Manually tweaking of the constants did not follow any sort of rigorous
method and was mainly done till the average and maximum errors "seemed better".
In tweaking the constants, there were two goals: improve the average error and
improve the maximum error. With sine and cosine, these two goals were at odds
with each other. In improving the average error, the maximum error would get
worse, or vice-versa. Choosing a good compromise took human intuition.

The graph below shows the errors in approximation using the tweaked constants
that we settled upon. The two horizontal green lines represent the values *+ε*
and *-ε*. The red and blue lines that are smooth and solid represent the error
when using a floating-point approximation. Note that the maximum of these errors
almost lie entirely within *±ε*. Lastly, the red and blue shaded regions
represent the error when using the fixed-point approximation. These errors
generally follow the trend of the floating-point error, but are worse due to
truncation errors.

![chart-error](http://code.digital-static.net/tri-approx/raw/tip/doc/chart-error.png)

Using a short C program to exhaustively compute all values in the input domain,
we could compute the average and maximum errors of our fixed-point
approximations. The sine approximation had an average error of (3.08±2.22)E-6
and a maximum error of 13.8E-6. The cosine approximation had an average error
of (2.82±2.05)E-6 and a maximum error of 12.6E-6.

Given that the cosine design takes less hardware resources and is actually more
accurate, it may make sense to implement cosine over sine. Sine itself can be
obtained from cosine by shifting the input by 90°.


## References ##

* [FPGA Multiplier](http://www.altera.com/literature/hb/cyc2/cyc2_cii51012.pdf) - Embedded multipliers on Cyclone II devices
* [Taylor Series](http://en.wikipedia.org/wiki/Taylor_series) - Wikipedia article on Taylor series
* [CORDIC](http://en.wikipedia.org/wiki/CORDIC) - Wikipedia article on CORDIC
