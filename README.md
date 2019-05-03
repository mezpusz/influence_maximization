Multi-threaded implementation of the [Influence Maximization Problem](https://www.cs.cornell.edu/home/kleinber/kdd03-inf.pdf),
using the independent cascade model and based on the greedy lazy-evaluation CELF++ algorithm of [this paper](http://snap.stanford.edu/class/cs224w-readings/goyal11celf.pdf).

## Requirements for Unix

cmake, make, clang, libc++-dev, libc++abi-dev

## Building it locally

Linux/Mac:

````bash
./build.sh
````

Windows:

````bash
cmake . -G "Visual Studio <version>"
````

where <version> is your Visual Studio version (can be 14, 15, 16, etc.).
Then open the `.sln` file and build the project.

## Arguments

* `-i`: Input file location
* `-o`: Ouput file location
* `-k`: Output set size (default is 10)
* `--it`: Number of Monte-Carlo iterations in a simulation round (default is 10000)
* `-p`: Probability of a node influencing a neighbor (default is 0.1)
* `--threads`: Number of threads to run on (default is std::thread::hardware_concurrency())

## Input format

The input file should contain all the node-node pairs (edges) that are present in the graph.
Note that this way, no isolated edges can be given (but they are not much use in this problem
either). All nodes can be distinguished only by a non-negative integer id.
The file can contain commented lines starting with `#`.
See example file `CA-HepTh.txt`.
