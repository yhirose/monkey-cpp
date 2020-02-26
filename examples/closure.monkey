// `newAdder` returns a closure that makes use of the free variables `a` and `b`:
let newAdder = fn(a, b) {
    fn(c) { a + b + c };
};

// This constructs a new `adder` function:
let adder = newAdder(1, 2);

puts(adder(8)); // => 11
