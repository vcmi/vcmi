assert(type(collectgarbage) == "nil", "collectgarbage enabled")
assert(type(dofile) == "nil", "dofile enabled")
assert(type(load) == "nil", "load enabled")
assert(type(loadfile) == "nil", "loadfile enabled")
assert(type(loadstring) == "nil", "loadstring enabled")
assert(type(print) == "nil", "print enabled")
assert(type(module) == "nil", "module enabled")

assert(type(string) == "table")
assert(type(string.dump) == "nil", "string.dump enabled")

assert(type(math) == "table")
assert(type(math.random) == "nil", "math.random enabled")
assert(type(math.randomseed) == "nil", "math.randomseed enabled")

assert(type(io) == "nil", "io enabled")
assert(type(file) == "nil", "io.file enabled")
assert(type(os) == "nil", "os enabled")
assert(type(debug) == "nil", "debug enabled")
assert(type(package) == "nil", "package enabled")

