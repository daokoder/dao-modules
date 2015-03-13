## statistics -- basic statistical measurements

The module contains functions for computing basic statistical measures of samples of data

### Index
namespace [stat](#std)

Functions:
- [mean](#mean)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;) => float
- [variance](#variance)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;, _kind_: enum&lt;sample,population&gt; = $sample) => float
- [variance](#variance)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;, _mean_: float, _kind_: enum&lt;sample,population&gt; = $sample) => float
- [median](#median)(_data_: array&lt;@T&lt;int|float&gt;&gt;) => float
- [percentile](#percentile)(_data_: array&lt;@T&lt;int|float&gt;&gt;, _percentage_: float) => float
- [mode](#mode)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;) => @T
- [range](#range)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;) => tuple&lt;min: @T, max: @T&gt;
- [distribution](#distribution1)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;) => map&lt;@T,int&gt;
- [distribution](#distribution2)(invar _data_: array&lt;@T&gt;int|float&gt;&gt;, _interval_: float, _start_ = 0.0) => map&lt;int,int&gt;
- [correlation](#correlation)(invar _data1_: array&lt;@T&lt;int|float&gt;&gt;, invar _data2_: array&lt;@T&gt;, _coefficient_: enum&lt;pearson,spearman&gt;) => float
- [correlation](#correlation)(invar _data1_: array&lt;@T&lt;int|float&gt;&gt;, invar _data2_: array&lt;@T&gt;, _coefficient_: enum&lt;pearson,spearman&gt;, _mean1_: float, _mean2_: float) => float
- [skewness](#skewness)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;) => float
- [skewness](#skewness)(invar data: array&lt;@T&lt;int|float&gt;&gt;, mean: float) => float
- [kurtosis](#kurtosis)(invar data: array&lt;@T&lt;int|float&gt;&gt;) => float
- [kurtosis](#kurtosis)(invar _data_: array&lt;@T&lt;int|float&gt;&gt;, _mean_: float) => float

<a name="stat"></a>
### Functions
<a name="mean"></a>
```ruby
mean(invar data: array<@T<int|float>>) => float
```
Returns arithmetic mean of *data*.

E[X] = Σx / N
<a name="variance"></a>
```ruby
variance(invar data: array<@T<int|float>>, kind: enum<sample,population> = $sample) => float
variance(invar data: array<@T<int|float>>, mean: float, kind: enum<sample,population> = $sample) => float
```
Returns variance of *data* (measure of spread) of the given *kind*. Uses *mean* if it is given.

Sample:		σ²[X] = Σ(x - E[X]) / (N - 1)
Population:	σ²[X] = Σ(x - E[X]) / N
<a name="median"></a>
```ruby
median(data: array<@T<int|float>>) => float
```
Returns median (middle value) of *data* while partially sorting it. If *data* size is even, the mean of two middle values is returned
<a name="percentile"></a>
```ruby
percentile(data: array<@T<int|float>>, percentage: float) => float
```
Returns percentile *percentage* (the value below which the given percentage of sample values fall) of *data* while partially sorting it. *percentage* must be
in range (0; 100)
<a name="mode"></a>
```ruby
mode(invar data: array<@T<int|float>>) => @T
```
Returns mode (most common value) of *data*
<a name="range"></a>
```ruby
range(invar data: array<@T<int|float>>) => tuple<min: @T, max: @T>
```
Returns minimum and maximum value in *data*
<a name="distribution1"></a>
```ruby
distribution(invar data: array<@T<int|float>>) => map<@T,int>
```
Returns distribution of values in *data* in the form `value` => `frequency`, where `value` is a single unique value and `frequency` is the number of its appearances
in *data*
<a name="distribution2"></a>
```ruby
distribution(invar data: array<@T<int|float>>, interval: float, start = 0.0) => map<int,int>
```
Returns values of *data* grouped into ranges of width *interval* starting from *start*. The result is in the form `index` => `frequency` corresponding to the ranges
present in the sample. `index` identifies the range, it is equal to integer number of intervals *interval* between *start* and the beginning of the particular range;
the exact range boundaries are [*start* + `floor`(`index` / *interval*); *start* + `floor`(`index` / *interval*) + *interval*). `frequency` is the number of values
which fall in the range. The values lesser then *start* are not included in the resulting statistics
<a name="correlation"></a>
```ruby
correlation(invar data1: array<@T<int|float>>, invar data2: array<@T>, coefficient: enum<pearson,spearman>) => float
correlation(invar data1: array<@T<int|float>>, invar data2: array<@T>, coefficient: enum<pearson,spearman>, mean1: float, mean2: float) => float
```
Returns correlation *coefficient* between *data1* and *data2*. Pearson coefficient measures linear dependence, Spearman's rank coefficient measures monotonic dependence.
If *mean1* and *mean2* are given, they are used for calculating Pearson coefficient.

**Note:** *self* and *other* must be of equal size

Pearson:			r[X,Y] = E[(X - E[X])(Y - E[Y])] / σ[X]σ[Y]
Spearman's rank:	ρ[X,Y] = r(Xrank, Yrank)
<a name="skewness"></a>
```ruby
skewness(invar data: array<@T<int|float>>) => float
skewness(invar data: array<@T<int|float>>, mean: float) => float
```
Returns skewness (measure of asymmetry) of *data*. Uses *mean* if it is given.

γ1[X] = E[((x - E[X]) / σ)^3]
<a name="kurtosis"></a>
```ruby
kurtosis(invar data: array<@T<int|float>>) => float
kurtosis(invar data: array<@T<int|float>>, mean: float) => float
```
Returns kurtosis (measure of "peakedness"). Uses *mean* if it is given

γ2[X] = E[((x - E[X]) / σ)^4] - 3
