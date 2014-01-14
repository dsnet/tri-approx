# Fixed-point Sine and Cosine Approximator #

## Introduction ##

Many simple embedded applications (microcontrollers, FPGAs, etc) do not come
with integrated floating-point units. However, the need to


Description: A quick test to verify an algorithm to compute fixed point values
of basic trigonometric functions using Taylor series. Given that the algorithm
functions properly and and accurately,
This method was chosen as
opposed to CORDIC because a

## Theory ##

## Results ##

## Frequently asked questions ##

### Why not use the CORDIC approximation? ###

CORDIC is a great algorithm for this application. Unfortunately, CORDIC uses
a relatively large lookup table of constants to operate. This table occupied
more RAM than I was willing to sacrifice for my application.

## References ##

* [Taylor series](http://en.wikipedia.org/wiki/Taylor_series) - Wikipedia article on Taylor series
* [CORDIC](http://en.wikipedia.org/wiki/CORDIC) - Wikipedia article on CORDIC
