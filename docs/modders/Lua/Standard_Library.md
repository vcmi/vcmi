# Lua Standard Library

VCMI supports small subset of Lua standard library for scripts to use. This page contains copy of relevant parts of official Lua docs for convenience.

## Basic Functions

The basic library provides some core functions to Lua. If you do not include this library in your application, you should check carefully whether you need to provide implementations for some of its facilities.

- `assert (v [, message])` - Issues an error when the value of its argument v is false (i.e., nil or false); otherwise, returns all its arguments. message is an error message; when absent, it defaults to "assertion failed!"
- `error (message [, level])` - Terminates the last protected function called and returns message as the error message. Function error never returns. Usually, error adds some information about the error position at the beginning of the message. The level argument specifies how to get the error position. With level 1 (the default), the error position is where the error function was called. Level 2 points the error to where the function that called error was called; and so on. Passing a level 0 avoids the addition of error position information to the message.
- `_G` - A global variable (not a function) that holds the global environment (that is, _G._G = _G). Lua itself does not use this variable; changing its value does not affect any environment, nor vice-versa. (Use setfenv to change environments.)
- `getfenv ([f])` - Returns the current environment in use by the function. f can be a Lua function or a number that specifies the function at that stack level: Level 1 is the function calling getfenv. If the given function is not a Lua function, or if f is 0, getfenv returns the global environment. The default for f is 1.
- `getmetatable (object)` - If object does not have a metatable, returns nil. Otherwise, if the object's metatable has a "__metatable" field, returns the associated value. Otherwise, returns the metatable of the given object.
- `ipairs (t)` - Returns three values: an iterator function, the table t, and 0, so that the construction `for i,v in ipairs(t) do body end` will iterate over the pairs `(1,t[1]), (2,t[2]), ···,` up to the first integer key absent from the table.
- `next (table [, index])` - Allows a program to traverse all fields of a table. Its first argument is a table and its second argument is an index in this table. next returns the next index of the table and its associated value. When called with nil as its second argument, next returns an initial index and its associated value. When called with the last index, or with nil in an empty table, next returns nil. If the second argument is absent, then it is interpreted as nil. In particular, you can use next(t) to check whether a table is empty. The order in which the indices are enumerated is not specified, even for numeric indices. (To traverse a table in numeric order, use a numerical for or the ipairs function.) The behavior of next is undefined if, during the traversal, you assign any value to a non-existent field in the table. You may however modify existing fields. In particular, you may clear existing fields.
- `pairs (t)` - Returns three values: the next function, the table t, and nil, so that the construction `for k,v in pairs(t) do body end` will iterate over all key–value pairs of table t. See function next for the caveats of modifying the table during its traversal.
- `pcall (f, arg1, ···)` - Calls function f with the given arguments in protected mode. This means that any error inside f is not propagated; instead, pcall catches the error and returns a status code. Its first result is the status code (a boolean), which is true if the call succeeds without errors. In such case, pcall also returns all results from the call, after this first result. In case of any error, pcall returns false plus the error message.`
- `print (···)` - Receives any number of arguments, and prints their values to stdout, using the tostring function to convert them to strings. print is not intended for formatted output, but only as a quick way to show a value, typically for debugging. For formatted output, use string.format.
- `rawequal (v1, v2)` - Checks whether v1 is equal to v2, without invoking any metamethod. Returns a boolean.
- `rawget (table, index)` - Gets the real value of `table[index]`, without invoking any metamethod. table must be a table; index may be any value.
- `rawset (table, index, value)` - Sets the real value of `table[index]` to value, without invoking any metamethod. table must be a table, index any value different from nil, and value any Lua value. This function returns table.
- `select (index, ···)` - If index is a number, returns all arguments after argument number index. Otherwise, index must be the string "#", and select returns the total number of extra arguments it received.
- `setfenv (f, table)` - Sets the environment to be used by the given function. f can be a Lua function or a number that specifies the function at that stack level: Level 1 is the function calling setfenv. setfenv returns the given function. As a special case, when f is 0 setfenv changes the environment of the running thread. In this case, setfenv returns no values.
- `setmetatable (table, metatable)` - Sets the metatable for the given table. (You cannot change the metatable of other types from Lua, only from C.) If metatable is nil, removes the metatable of the given table. If the original metatable has a `"__metatable"` field, raises an error. This function returns table.
- `tonumber (e [, base])` - Tries to convert its argument to a number. If the argument is already a number or a string convertible to a number, then tonumber returns this number; otherwise, it returns nil. An optional argument specifies the base to interpret the numeral. The base may be any integer between 2 and 36, inclusive. In bases above 10, the letter 'A' (in either upper or lower case) represents 10, 'B' represents 11, and so forth, with 'Z' representing 35. In base 10 (the default), the number can have a decimal part, as well as an optional exponent part (see §2.1). In other bases, only unsigned integers are accepted.
- `tostring (e)` - Receives an argument of any type and converts it to a string in a reasonable format. For complete control of how numbers are converted, use string.format. If the metatable of e has a "__tostring" field, then tostring calls the corresponding value with e as argument, and uses the result of the call as its result.
- `type (v)` - Returns the type of its only argument, coded as a string. The possible results of this function are "nil" (a string, not the value nil), "number", "string", "boolean", "table", "function", "thread", and "userdata".
- `unpack (list [, i [, j]])` - Returns the elements from the given table. This function is equivalent to `return list[i], list[i+1], ···, list[j]` except that the above code can be written only for a fixed number of elements. By default, i is 1 and j is the length of the list, as defined by the length operator.
- `_VERSION` - A global variable (not a function) that holds a string containing the current interpreter version. The current contents of this variable is "Lua 5.1".
- `xpcall (f, err)` - This function is similar to pcall, except that you can set a new error handler. `xpcall` calls function f in protected mode, using err as the error handler. Any error inside f is not propagated; instead, xpcall catches the error, calls the err function with the original error object, and returns a status code. Its first result is the status code (a boolean), which is true if the call succeeds without errors. In this case, xpcall also returns all results from the call, after this first result. In case of any error, xpcall returns false plus the result from err.

## Table Manipulation

This library provides generic functions for table manipulation. It provides all its functions inside the table table.

Most functions in the table library assume that the table represents an array or a list. For these functions, when we talk about the "length" of a table we mean the result of the length operator.

- `table.concat (table [, sep [, i [, j]]])` - Given an array where all elements are strings or numbers, returns table[i]..sep..table[i+1] ··· sep..table[j]. The default value for sep is the empty string, the default for i is 1, and the default for j is the length of the table. If i is greater than j, returns the empty string.
- `table.insert (table, [pos,] value)` - Inserts element value at position pos in table, shifting up other elements to open space, if necessary. The default value for pos is n+1, where n is the length of the table (see §2.5.5), so that a call table.insert(t,x) inserts x at the end of table t.
- `table.maxn (table)` - Returns the largest positive numerical index of the given table, or zero if the table has no positive numerical indices. (To do its job this function does a linear traversal of the whole table.)
- `table.remove (table [, pos])` - Removes from table the element at position pos, shifting down other elements to close the space, if necessary. Returns the value of the removed element. The default value for pos is n, where n is the length of the table, so that a call table.remove(t) removes the last element of table t.
- `table.sort (table [, comp])` - Sorts table elements in a given order, in-place, from table[1] to table[n], where n is the length of the table. If comp is given, then it must be a function that receives two table elements, and returns true when the first is less than the second (so that not comp(a[i+1],a[i]) will be true after the sort). If comp is not given, then the standard Lua operator < is used instead. The sort algorithm is not stable; that is, elements considered equal by the given order may have their relative positions changed by the sort.

## Mathematical Functions

This library is an interface to the standard C math library. It provides all its functions inside the table math.

- `math.abs (x)` - Returns the absolute value of x.
- `math.acos (x)` - Returns the arc cosine of x (in radians).
- `math.asin (x)` - Returns the arc sine of x (in radians).
- `math.atan (x)` - Returns the arc tangent of x (in radians).
- `math.atan2 (y, x)` - Returns the arc tangent of y/x (in radians), but uses the signs of both parameters to find the quadrant of the result. (It also handles correctly the case of x being zero.)
- `math.ceil (x)` - Returns the smallest integer larger than or equal to x.
- `math.cos (x)` - Returns the cosine of x (assumed to be in radians).
- `math.cosh (x)` - Returns the hyperbolic cosine of x.
- `math.deg (x)` - Returns the angle x (given in radians) in degrees.
- `math.exp (x)` - Returns the value ex.
- `math.floor (x)` - Returns the largest integer smaller than or equal to x.
- `math.fmod (x, y)` - Returns the remainder of the division of x by y that rounds the quotient towards zero.
- `math.frexp (x)` - Returns m and e such that x = m2e, e is an integer and the absolute value of m is in the range `[0.5, 1)` (or zero when x is zero).
- `math.huge` - The value HUGE_VAL, a value larger than or equal to any other numerical value.
- `math.ldexp (m, e)` - Returns m2e (e should be an integer).
- `math.log (x)` - Returns the natural logarithm of x.
- `math.log10 (x)` - Returns the base-10 logarithm of x.
- `math.max (x, ···)` - Returns the maximum value among its arguments.
- `math.min (x, ···)` - Returns the minimum value among its arguments.
- `math.modf (x)` - Returns two numbers, the integral part of x and the fractional part of x.
- `math.pi` - The value of pi.
- `math.pow (x, y)` - Returns xy. (You can also use the expression x^y to compute this value.)
- `math.rad (x)` - Returns the angle x (given in degrees) in radians.
- `math.sin (x)` - Returns the sine of x (assumed to be in radians).
- `math.sinh (x)` - Returns the hyperbolic sine of x.
- `math.sqrt (x)` - Returns the square root of x. (You can also use the expression x^0.5 to compute this value.)
- `math.tan (x)` - Returns the tangent of x (assumed to be in radians).
- `math.tanh (x)` - Returns the hyperbolic tangent of x.

## String Manipulation

This library provides generic functions for string manipulation, such as finding and extracting substrings, and pattern matching. When indexing a string in Lua, the first character is at position 1 (not at 0, as in C). Indices are allowed to be negative and are interpreted as indexing backwards, from the end of the string. Thus, the last character is at position -1, and so on.

The string library provides all its functions inside the table string. It also sets a metatable for strings where the __index field points to the string table. Therefore, you can use the string functions in object-oriented style. For instance, string.byte(s, i) can be written as s:byte(i).

The string library assumes one-byte character encodings.

- `string.byte (s [, i [, j]])` - Returns the internal numerical codes of the characters `s[i], s[i+1], ···, s[j]`. The default value for i is 1; the default value for j is i. Note that numerical codes are not necessarily portable across platforms.
- `string.char (···)` - Receives zero or more integers. Returns a string with length equal to the number of arguments, in which each character has the internal numerical code equal to its corresponding argument. Note that numerical codes are not necessarily portable across platforms.
- `string.find (s, pattern [, init [, plain]])` - Looks for the first match of pattern in the string s. If it finds a match, then find returns the indices of s where this occurrence starts and ends; otherwise, it returns nil. A third, optional numerical argument init specifies where to start the search; its default value is 1 and can be negative. A value of true as a fourth, optional argument plain turns off the pattern matching facilities, so the function does a plain "find substring" operation, with no characters in pattern being considered "magic". Note that if plain is given, then init must be given as well. If the pattern has captures, then in a successful match the captured values are also returned, after the two indices.
- `string.format (formatstring, ···)` - Returns a formatted version of its variable number of arguments following the description given in its first argument (which must be a string). The format string follows the same rules as the printf family of standard C functions. The only differences are that the options/modifiers *, l, L, n, p, and h are not supported and that there is an extra option, q. The q option formats a string in a form suitable to be safely read back by the Lua interpreter: the string is written between double quotes, and all double quotes, newlines, embedded zeros, and backslashes in the string are correctly escaped when written. For instance, the call:

  ```lua
    string.format('%q', 'a string with "quotes" and \n new line')
  ```

  will produce the string:

  ```text
    "a string with \"quotes\" and \
     new line"
  ```

  The options c, d, E, e, f, g, G, i, o, u, X, and x all expect a number as argument, whereas q and s expect a string.

  This function does not accept string values containing embedded zeros, except as arguments to the q option.

- `string.gmatch (s, pattern)` - Returns an iterator function that, each time it is called, returns the next captures from pattern over string s. If pattern specifies no captures, then the whole match is produced in each call.

  As an example, the following loop:

  ```lua
    s = "hello world from Lua"
    for w in string.gmatch(s, "%a+") do
      print(w)
    end
  ```

  will iterate over all the words from string s, printing one per line. The next example collects all pairs key=value from the given string into a table:

  ```lua
    t = {}
    s = "from=world, to=Lua"
    for k, v in string.gmatch(s, "(%w+)=(%w+)") do
      t[k] = v
    end
  ```

  For this function, a '^' at the start of a pattern does not work as an anchor, as this would prevent the iteration.

- `string.gsub (s, pattern, repl [, n])` - Returns a copy of s in which all (or the first n, if given) occurrences of the pattern have been replaced by a replacement string specified by repl, which can be a string, a table, or a function. gsub also returns, as its second value, the total number of matches that occurred.

  If repl is a string, then its value is used for replacement. The character % works as an escape character: any sequence in repl of the form %n, with n between 1 and 9, stands for the value of the n-th captured substring (see below). The sequence %0 stands for the whole match. The sequence %% stands for a single %.

  If repl is a table, then the table is queried for every match, using the first capture as the key; if the pattern specifies no captures, then the whole match is used as the key.

  If repl is a function, then this function is called every time a match occurs, with all captured substrings passed as arguments, in order; if the pattern specifies no captures, then the whole match is passed as a sole argument.

  If the value returned by the table query or by the function call is a string or a number, then it is used as the replacement string; otherwise, if it is false or nil, then there is no replacement (that is, the original match is kept in the string).

  Here are some examples:
  ```lua
    x = string.gsub("hello world", "(%w+)", "%1 %1")
    --> x="hello hello world world"
     
    x = string.gsub("hello world", "%w+", "%0 %0", 1)
    --> x="hello hello world"
     
    x = string.gsub("hello world from Lua", "(%w+)%s*(%w+)", "%2 %1")
    --> x="world hello Lua from"
     
    x = string.gsub("home = $HOME, user = $USER", "%$(%w+)", os.getenv)
    --> x="home = /home/roberto, user = roberto"
     
    x = string.gsub("4+5 = $return 4+5$", "%$(.-)%$", function (s)
          return loadstring(s)()
        end)
    --> x="4+5 = 9"
     
    local t = {name="lua", version="5.1"}
    x = string.gsub("$name-$version.tar.gz", "%$(%w+)", t)
    --> x="lua-5.1.tar.gz"
  ```
- `string.len (s)` - Receives a string and returns its length. The empty string "" has length 0. Embedded zeros are counted, so "a\000bc\000" has length 5.
- `string.lower (s)` - Receives a string and returns a copy of this string with all uppercase letters changed to lowercase. All other characters are left unchanged. The definition of what an uppercase letter is depends on the current locale.
- `string.match (s, pattern [, init])` - Looks for the first match of pattern in the string s. If it finds one, then match returns the captures from the pattern; otherwise it returns nil. If pattern specifies no captures, then the whole match is returned. A third, optional numerical argument init specifies where to start the search; its default value is 1 and can be negative.
- `string.rep (s, n)` - Returns a string that is the concatenation of n copies of the string s.
- `string.reverse (s)` - Returns a string that is the string s reversed.
- `string.sub (s, i [, j])` - Returns the substring of s that starts at i and continues until j; i and j can be negative. If j is absent, then it is assumed to be equal to -1 (which is the same as the string length). In particular, the call string.sub(s,1,j) returns a prefix of s with length j, and string.sub(s, -i) returns a suffix of s with length i.
- `string.upper (s)` - Receives a string and returns a copy of this string with all lowercase letters changed to uppercase. All other characters are left unchanged. The definition of what a lowercase letter is depends on the current locale.
