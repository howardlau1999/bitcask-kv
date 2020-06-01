## Build
 
```
bazel build :simplekvsvr
```

## Run

Run the built binary with no arguments. It will store data in current working directory and serve on port 7777.

## Usage

The protocol is text-based and `\n`-separated. Requests begin with `-` and responses begin with `+`.

### Get

```
-get key\n
+value\n
```

### Set

```
-set key value\n
+OK
```

### Delete

```
-delete key\n
+OK\n
```

### Stats

```
-stats\n
+count: 0, mem: 1460 KB, file: 0 B, hits: 0, misses: 0, cache hits: 0, cache misses: 0\n
```

### Quit

```
-quit\n
+OK\n
```

### Compact

```
-compact\n
+OK\n
```

## Benchmark

```
bazel build :simplekvsvr_benchmark
```

Then run benchmark with 5 arguments

```
Usage: simplekvsvr_benchmark threadCount operationsPerTheard keySize(byte) valueSize(byte) r/w
```

The 5th argument `r` means testing GET operations and `w` means testing SET operations. You have to run `w` first to load data into KV and save random generated KV pairs in `testdata` dir before you run `r` tests.
