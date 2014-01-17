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

Since this experiment was targetted towards Altera FPGAs, the bit-width of 18
will appear frequently in the example implementation. This is due to the fact
that Altera hardware multipliers have 18b wide inputs and have 36b wide outputs.
As such the sine and cosine functions will take in a 20b unsigned fixed-point
integer upscaled by 2²⁰ and output a 18b two's complement fixed-point integer
upscaled by 2¹⁷. Notice that the domain of the input is [0,1) and the range of
the output is [-1,+1). This is because the sine and cosine functions implemented
here are normalized such that 0 maps to 0.0 and 2π maps to 1.0. In other words,
the functions being implemented are actually sine(2πx) and cosine(2πx).

## Theory ##

*Mention Taylor series*
![eqn-taylor-series](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-taylor-series.png)

*Mention how this is for normalized sine and cosine*
![eqn-sine](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-sine.png)

![eqn-cosine](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-cosine.png)

*Mention how reflection is used*

*Mention how only 4 constants are used*
![eqn-constants](http://code.digital-static.net/tri-approx/raw/tip/doc/eqn-constants.png)


The following Python code demonstrates how the

```python
for n in range(1,9):
	val = (2*math.pi)**n/math.factorial(n)
	scale = 0
	while val*2**(scale+1) < 2**18:
		scale += 1
	print n, int(round(val*2**scale)), scale
```

## Results ##

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
