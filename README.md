# filang

A lightweight bytecode virtual machine and single-pass Pratt parser written from scratch in C.

---

### Why I built this

Back in 2023, during my final year of high school, I wanted to stop treating compilers and runtime environments as black boxes and really understand what happens under the hood when code executes. I picked up Robert Nystrom's [*Crafting Interpreters*](https://craftinginterpreters.com/) and decided to build my own language in pure C.

Rather than just following along and copying the book's `clox` implementation line-by-line, I used it as an architectural foundation to experiment with my own ideas, test custom data structures, and solve low-level systems problems myself.

Looking back, building this in high school completely demystified memory layouts, pointers, and CPU-level execution, shaping how I think about systems engineering to this day.

---

### Syntax

`filang` uses a clean, lightweight syntax with first-class support for both integers and floating-point math:

```filang
# Variable declarations start with ':'
: language = "filang";
: version = 1;
print "Running " + language + " v" + version;

# Math & Exponentiation (**)
: base = 2;
: exp = 10;
print "2 ** 10 = " + (base ** exp); # 1024

# Bitwise Operations (&, |, ^, ~, <<, >>)
: mask = 255;
print "Masked: " + (mask & 15);     # 15
print "Shifted: " + (1 << 4);       # 16

# Runtime Type Inspection
print "typeof base: " + typeof base; # <builtin 'integer'>

# Conditional Branching (? and :)
? (base < exp) {
    print "base is smaller than exp";
} : {
    print "base is greater or equal";
}
```

---

### What I did differently from *clox*

While the single-pass Pratt parser and bytecode dispatch loop draw inspiration from Nystrom's design, roughly half of the codebase is custom or rewritten with different architectural trade-offs:

* **Robin Hood Hashing instead of Linear Probing (`hashmap.c`)**:  
  The book uses a basic open-addressing table with tombstone markers for deletion. I wanted something more optimal, so I implemented a **Robin Hood hash map** with Probe Sequence Length (PSL) tracking. On collisions, entries are swapped if the incoming key has traveled farther (`cur_dist < dist`), keeping variance low. Deletions use **backward-shift displacement**, completely eliminating the need for tombstone sentinels.
* **Real 64-bit Integers alongside Floats (`value.h` / `vm.c`)**:  
  It always bothered me that languages like JavaScript (and `clox`) treat every number as an IEEE 754 float. I added distinct `int64_t` support alongside `double`. Pure integer operations stay bitwise-exact and fast, while mixed operations and divisions promote dynamically to float.
* **Single-Allocation Strings via Flexible Array Members (`strings.c`)**:  
  Instead of doing two separate `malloc` calls for every string (one for the `ObjString` struct and one for the `char*` buffer), I took on one of the book's challenges: using C99 **Flexible Array Members** (`char chars[]`). The struct header and the string payload live in a single contiguous block of memory, improving CPU cache locality and halving allocator overhead.
* **Extended Opcodes & Peephole Lookahead**:  
  Added native bytecode instructions for bitwise operations (`&`, `|`, `^`, `~`, `<<`, `>>`), modulo (`%`), power (`**`), and ternary expressions. In the VM dispatch loop, comparison opcodes (`OP_GREATER`, `OP_LESS`) look ahead at the next byte: if followed by `OP_NOT`, they fuse into `<=` or `>=` at runtime, saving an entire dispatch cycle.
* **Compressed Source Line Mapping (`chunk.c`)**:  
  Instead of allocating a 4-byte integer per instruction byte to store debug line numbers, `filang` records only the bytecode offsets where each line ends (`chunk->lines.ends`), drastically cutting memory usage.
* **A Real REPL (`main.c`)**:  
  Integrated GNU Readline for command history (`add_history`), tab handling, clean `SIGINT` (Ctrl+C) trapping, and auto-printing evaluated expressions.

---

### Where it stopped

This project was a personal milestone. I stopped development after getting expressions, local/global variable scopes, and conditional branching working cleanly — right before tackling function call frames, closures, and a mark-sweep garbage collector.

---

### Building and Running

#### Prerequisites
* A C17-compatible compiler (`clang` or `gcc`)
* CMake (>= 3.20)
* GNU `readline` (standard on macOS and Linux)

#### Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

#### Run the interactive REPL
```bash
./build/filang
# fi>> : x = 42;
# fi>> print x * 2;
# 84
```

#### Run a script
```bash
./build/filang script.fi
```

---

### License

MIT License - feel free to inspect, fork, or learn from it! See [LICENSE](LICENSE) for details.
