# Optimization of the Gridding program on the CPU+MIC Platform

The code that is required to be optimized is the `gridKernel` function:

```
    const int sSize = 2 * support + 1;

    for (int dind = 0; dind < int(samples.size()); ++dind) {
        // The actual grid point from which we offset
        int gind = samples[dind].iu + gSize * samples[dind].iv - support;

        // The Convoluton function point from which we offset
        int cind = samples[dind].cOffset;

        for (int suppv = 0; suppv < sSize; suppv++) {
            Value* gptr = &grid[gind];
            const Value* cptr = &C[cind];
            const Value d = samples[dind].data;
            for (int suppu = 0; suppu < sSize; suppu++) {
                *(gptr++) += d * (*(cptr++));
            }

            gind += gSize;
            cind += sSize;
        }
    }
```

It contains 3 nested for loops, which can be paralleled in order to improve performace.

### Optimization of the for-loop in the first level

It's easy to see that the for-loop in the first level is independent from each other,
thus we can directly use OpenMP's `parallel for` feature. The resulting code may look
like this:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
        // The actual grid point from which we offset
        int gind = samples[dind].iu + gSize * samples[dind].iv - support;

        // The Convoluton function point from which we offset
        int cind = samples[dind].cOffset;

        for (int suppv = 0; suppv < sSize; suppv++) {
            Value* gptr = &grid[gind];
            const Value* cptr = &C[cind];
            const Value d = samples[dind].data;
            for (int suppu = 0; suppu < sSize; suppu++) {
                *(gptr++) += d * (*(cptr++));
            }

            gind += gSize;
            cind += sSize;
        }
    }
```

### Optimization of the for-loop in the second level

The second level's for-loop may seem dependent from the first glance. The variable `gptr`
and the pointer to the constant variable `cptr` depends on integers `gind` and `cind` in
the upper level, which is then modified inside the for-loop in the second level.

In order to optimize them, we first move integers `gind` and `cind` to the second level's
for-loop, since they are not used in the first level's for-loop:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
        for (int suppv = 0; suppv < sSize; suppv++) {
            int gind = samples[dind].iu + gSize * samples[dind].iv - support;
            int cind = samples[dind].cOffset;

            Value* gptr = &grid[gind];
            const Value* cptr = &C[cind];
            const Value d = samples[dind].data;
            for (int suppu = 0; suppu < sSize; suppu++) {
                *(gptr++) += d * (*(cptr++));
            }

            gind += gSize;
            cind += sSize;
        }
    }
```

**This doesn't work** because before the change, `gind` and `cind` are shared among all
for-loops, and they are increased in each of them. However, we can easily see the trend
that the amount of the increasement only depends on `suppv`. We change the last two
lines of the code to make it work:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
        for (int suppv = 0; suppv < sSize; suppv++) {
            int gind = samples[dind].iu + gSize * samples[dind].iv - support;
            int cind = samples[dind].cOffset;

            gind += gSize * suppv;
            cind += sSize * suppv;

            Value* gptr = &grid[gind];
            const Value* cptr = &C[cind];
            const Value d = samples[dind].data;
            for (int suppu = 0; suppu < sSize; suppu++) {
                *(gptr++) += d * (*(cptr++));
            }
        }
    }
```

In this way, we make the second level for-loop independent from each other. Let's use
OpenMP's `parallel for` feature again to make them parallel:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
#pragma omp parallel for
        for (int suppv = 0; suppv < sSize; suppv++) {
            int gind = samples[dind].iu + gSize * samples[dind].iv - support;
            int cind = samples[dind].cOffset;

            gind += gSize * suppv;
            cind += sSize * suppv;

            Value* gptr = &grid[gind];
            const Value* cptr = &C[cind];
            const Value d = samples[dind].data;
            for (int suppu = 0; suppu < sSize; suppu++) {
                *(gptr++) += d * (*(cptr++));
            }
        }
    }
```

### Optimization of the for-loop in the third level

With the experience of optimizing the second level for-loop, we can apply the same method
for the third level:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
#pragma omp parallel for
        for (int suppv = 0; suppv < sSize; suppv++) {
            int gind = samples[dind].iu + gSize * samples[dind].iv - support;
            int cind = samples[dind].cOffset;

            gind += gSize * suppv;
            cind += sSize * suppv;

            const Value d = samples[dind].data;
#pragma omp parallel for
            for (int suppu = 0; suppu < sSize; suppu++) {
                Value* gptr = &grid[gind];
                const Value* cptr = &C[cind];

                gptr += suppu;
                cptr += suppu;

                *(gptr) += d * (*(cptr));
            }
        }
    }
```

### Critical Part

The line `*(gptr) += d * (*(cptr))` modifies data outside the for-loop. They must run
atomically in order to prevent data from breaking:

```
    const int sSize = 2 * support + 1;

#pragma omp parallel for
    for (int dind = 0; dind < int(samples.size()); ++dind) {
#pragma omp parallel for
        for (int suppv = 0; suppv < sSize; suppv++) {
            int gind = samples[dind].iu + gSize * samples[dind].iv - support;
            int cind = samples[dind].cOffset;

            gind += gSize * suppv;
            cind += sSize * suppv;

            const Value d = samples[dind].data;
#pragma omp parallel for
            for (int suppu = 0; suppu < sSize; suppu++) {
                Value* gptr = &grid[gind];
                const Value* cptr = &C[cind];

                gptr += suppu;
                cptr += suppu;
#pragma omp critical
                *(gptr) += d * (*(cptr));
            }
        }
    }
```
