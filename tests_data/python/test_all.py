#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Exhaustive Python Syntax Test | Python 3.8 through 3.14
"""

from __future__ import annotations  # PEP 563 / PEP 649 (3.14 makes this default)

# ═══════════════════════════════════════════════════════════════════════
# 0. COMMENTS — single-line, inline, and docstrings
# ═══════════════════════════════════════════════════════════════════════

# This is a comment
x = 1  # inline comment
"""Module-level docstring (already above, but a second string-statement is legal)."""

# ═══════════════════════════════════════════════════════════════════════
# LITERALS
# ═══════════════════════════════════════════════════════════════════════

# --- Integers ---
dec       = 0
dec2      = 1_000_000
dec3      = 123_456_789
hex_lit   = 0xFF
hex_lit2  = 0XAB_CD
oct_lit   = 0o77
oct_lit2  = 0O77_77
bin_lit   = 0b1010
bin_lit2  = 0B1111_0000
big_int   = 999_999_999_999_999_999_999_999_999

# --- Floats ---
f1  = 3.14
f2  = 3.14_15_93
f3  = 10.
f4  = .001
f5  = 1e10
f6  = 1E-10
f7  = 3.14e+2
f8  = 1_000.000_1
f9  = 1_0e1_0
f10 = .1_2e3_4
f11 = 0.0

# --- Complex ---
c1 = 3j
c2 = 3.14j
c3 = 10.j
c4 = .001j
c5 = 1e10j
c6 = 1_000j
c7 = 3.14_15j

# --- Boolean & None ---
b1 = True
b2 = False
n  = None

# --- Strings ---
s1  = "hello"
s2  = 'hello'
s3  = "it's"
s4  = 'it"s'
s5  = "escape sequences: \n \t \r \\ \' \" \a \b \f \v \0 \x41 \u0041 \U00000041 \N{SNOWMAN}"
s6  = 'raw string follows'
s7  = r"raw \n \t"
s8  = R'raw \n \t'
s9  = """triple
double
quoted"""
s10 = '''triple
single
quoted'''
s11 = r"""raw triple double"""
s12 = r'''raw triple single'''
s13 = "adjacent " "string " "concatenation"
s14 = ("implicit "
       "concatenation "
       "across lines")
s15 = "\N{GREEK SMALL LETTER ALPHA}"
s16 = "\110\145\154\154\157"  # octal escapes

# --- Bytes ---
by1 = b"bytes"
by2 = B'BYTES'
by3 = b"\x00\xff"
by4 = rb"raw bytes"
by5 = bR"raw bytes"
by6 = Rb"raw bytes"
by7 = BR"raw bytes"
by8 = br"raw bytes"
by9 = b"""triple
bytes"""
by10 = b'''triple
bytes single'''
by11 = rb"""raw triple bytes"""

# --- Ellipsis ---
e = ...
e2 = Ellipsis


# ═══════════════════════════════════════════════════════════════════════
# F-STRINGS (3.6+, significant changes in 3.12)
# ═══════════════════════════════════════════════════════════════════════

name = "world"
value = 42

# Basic
fs1 = f"hello {name}"
fs2 = F"hello {name!r}"
fs3 = f"value={value!s}"
fs4 = f"value={value!a}"
fs5 = f"{value:#010x}"
fs6 = f"{value:.2f}"
fs7 = f"{'nested string'}"
fs8 = f"{{escaped braces}}"
fs9 = f"{'multi' + 'concat'}"
fs10 = f"{value:{'.2f'}}"  # nested f-string (3.12 relaxed)

# F-string = debugging (3.8+)
fs_debug1 = f"{name=}"
fs_debug2 = f"{value=!r}"
fs_debug3 = f"{value=:.2f}"
fs_debug4 = f"{name = }"  # spaces around =
fs_debug5 = f"{2 + 2 = }"

# 3.12: f-strings can contain the same quote as the outer string
# 3.12: f-strings can span multiple lines in the expression
# 3.12: f-strings can contain backslashes in the expression
# 3.12: f-strings can contain # comments in multi-line expressions
if False:
    fs_samequote = f"hello {"world"}"
    fs_nested_quotes = f"{'hello' + "world"}"
    fs_triple_in_f = f"result = {
        value  # this is a comment inside f-string (3.12)
    }"
    fs_backslash = f"newline: {chr(10)}"  # workaround style
    fs_multiline = f"result = {
        1 +
        2 +
        3
    }"
    # Deeply nested f-strings (3.12)
    fs_deep = f"{'='*10 + f' {name!r} ' + '='*10}"
    fs_deep2 = f"{f"{f"{name}"}"}"

# Triple-quoted f-strings
fs_triple1 = f"""
Hello {name},
Value is {value}.
"""
fs_triple2 = f'''
Hello {name},
Value is {value}.
'''

# ═══════════════════════════════════════════════════════════════════════
# T-STRINGS (3.14 — PEP 750)
# ═══════════════════════════════════════════════════════════════════════
if False:
    ts1 = t"hello {name}"
    ts2 = t"value={value!r}"
    ts3 = t"{value:#010x}"
    ts4 = t"{name=}"
    ts5 = t"""
    multiline
    template {name}
    """
    ts6 = t"hello {name + "!"}"  # reuse quotes (3.12 f-string rules apply)


# ═══════════════════════════════════════════════════════════════════════
# VARIABLES, ASSIGNMENT, AUGMENTED ASSIGNMENT
# ═══════════════════════════════════════════════════════════════════════

a = 1
a: int = 1            # annotated assignment
b: list[int]          # annotation without assignment
_private = 2
__dunder__ = 3
_0 = 4
café = 5              # unicode identifier
π = 3.14159
変数 = "variable"
ñ = 1

# Augmented assignment
a += 1
a -= 1
a *= 2
a //= 1
a /= 1
a %= 3
a **= 2
a &= 0xFF
a |= 0x00
a ^= 0xFF
a >>= 1
a <<= 1

# Multiple assignment
x = y = z = 0
x, y, z = 1, 2, 3

# Swap
x, y = y, x

# Tuple assignment
(a, b) = (1, 2)
[a, b] = [1, 2]


# ═══════════════════════════════════════════════════════════════════════
# WALRUS OPERATOR (3.8 — PEP 572)
# ═══════════════════════════════════════════════════════════════════════

data = [1, 2, 3, 4, 5]

# In while
import io
reader = io.StringIO("line1\nline2\n")
while (line := reader.readline()):
    pass

# In if
if (n := len(data)) > 3:
    pass

# In list comprehension
filtered = [y for x in data if (y := x * 2) > 4]

# In any/all
result = any((match := x) > 3 for x in data)

# Nested walrus
if (a := (b := 10) + 1):
    pass


# ═══════════════════════════════════════════════════════════════════════
# UNPACKING & STARRED EXPRESSIONS
# ═══════════════════════════════════════════════════════════════════════

# Star unpacking
first, *rest = [1, 2, 3, 4]
first, *mid, last = [1, 2, 3, 4, 5]
*init, last2 = [1, 2, 3]
(a, b), c = (1, 2), 3
[a, (b, c)] = [1, (2, 3)]

# Nested unpacking
((a, b), (c, d)) = ((1, 2), (3, 4))
[a, [b, [c, d]]] = [1, [2, [3, 4]]]

# Star in function call
def func(*args, **kwargs): pass
func(*[1, 2], *[3, 4])
func(**{"a": 1}, **{"b": 2})
func(*[1], *[3], **{"a": 2}, **{"b": 4})  # multiple unpacks

# Star in list/dict/set/tuple literals
lst = [*[1, 2], *[3, 4], 5]
tup = (*[1, 2], *[3, 4], 5)
st  = {*[1, 2], *[3, 4], 5}
dct = {**{"a": 1}, **{"b": 2}, "c": 3}

# Underscore as throwaway
_, important, _ = (1, 2, 3)
_, *__ = [1, 2, 3, 4]

# ═══════════════════════════════════════════════════════════════════════
# OPERATORS & EXPRESSIONS
# ═══════════════════════════════════════════════════════════════════════

# Arithmetic
_ = 1 + 2
_ = 1 - 2
_ = 1 * 2
_ = 1 / 2
_ = 1 // 2
_ = 1 % 2
_ = 2 ** 10
_ = -1
_ = +1
_ = ~0

# Bitwise
_ = 0xFF & 0x0F
_ = 0xFF | 0x0F
_ = 0xFF ^ 0x0F
_ = 0xFF << 4
_ = 0xFF >> 4

# Comparison
_ = 1 == 1
_ = 1 != 2
_ = 1 < 2
_ = 1 > 2
_ = 1 <= 2
_ = 1 >= 2

# Chained comparison
_ = 1 < 2 < 3
_ = 1 < 2 <= 3 < 4
_ = 1 == 1 == 1

# Identity & membership
_ = 1 is 1
_ = 1 is not 2
_ = 1 in [1, 2]
_ = 1 not in [3, 4]

# Boolean
_ = True and False
_ = True or False
_ = not True

# Conditional (ternary)
_ = "yes" if True else "no"
_ = ("a" if True else "b") if False else ("c" if True else "d")

# Dict merge operators (3.9 — PEP 584)
d1 = {"a": 1}
d2 = {"b": 2}
d3 = d1 | d2
d1 |= d2

# Matrix multiply operator
class M:
    def __matmul__(self, other): return self
    def __imatmul__(self, other): return self
m = M()
_ = m @ m
m @= m


# ═══════════════════════════════════════════════════════════════════════
# CONTROL FLOW
# ═══════════════════════════════════════════════════════════════════════

# --- if / elif / else ---
if True:
    pass
elif False:
    pass
else:
    pass

# One-liner
if True: pass

# --- for loop ---
for i in range(10):
    pass
else:
    pass  # for-else

for i in range(10):
    if i == 5:
        break
    if i == 3:
        continue

# One-liner
for i in range(3): pass

# --- while loop ---
i = 0
while i < 10:
    i += 1
else:
    pass  # while-else

# One-liner
while False: pass

# --- Nested loops with break/continue ---
for i in range(3):
    for j in range(3):
        if j == 1:
            continue
        if i == 2:
            break

# --- pass, break, continue ---
for _ in range(1):
    pass
    break

# ═══════════════════════════════════════════════════════════════════════
# MATCH / CASE — Structural Pattern Matching (3.10 — PEP 634-636)
# ═══════════════════════════════════════════════════════════════════════

command = ("move", 10, 20)

# Literal patterns
match 42:
    case 0:
        pass
    case 42:
        pass
    case -1:
        pass
    case 3.14:
        pass
    case 1 + 2j:
        pass
    case True:
        pass
    case False:
        pass
    case None:
        pass
    case "hello":
        pass

# Capture patterns & wildcard
match command:
    case x:
        pass

match command:
    case _:
        pass

# Sequence patterns
match command:
    case []:
        pass
    case [x]:
        pass
    case [x, y]:
        pass
    case [x, *rest]:
        pass
    case [x, y, *_]:
        pass
    case (x, y, z):
        pass
    case [first, *middle, last]:
        pass

# Mapping patterns
d = {"action": "move", "x": 10}
match d:
    case {}:
        pass
    case {"action": "move"}:
        pass
    case {"action": action}:
        pass
    case {"action": "move", **rest}:
        pass
    case {"x": 10, "y": int(y)}:
        pass

# Class patterns
class Point:
    __match_args__ = ("x", "y")
    def __init__(self, x, y):
        self.x = x
        self.y = y

p = Point(1, 2)
match p:
    case Point(x=0, y=0):
        pass
    case Point(0, y):
        pass
    case Point(x, y) if x > 0:  # guard
        pass
    case Point(x=int(a), y=int(b)):
        pass

# OR patterns
match 42:
    case 1 | 2 | 3:
        pass
    case 40 | 41 | 42:
        pass

# AS patterns
match command:
    case ("move", x, y) as cmd:
        pass

# Guard clauses
match command:
    case ("move", x, y) if x > 0 and y > 0:
        pass
    case ("move", x, y) if x == y:
        pass

# Nested patterns
match {"users": [{"name": "Alice", "age": 30}]}:
    case {"users": [{"name": str(name), "age": int(age)}, *rest]}:
        pass

# Walrus in guard (legal)
match 42:
    case x if (doubled := x * 2) > 50:
        pass

# Dotted names (value patterns)
import os
match os.name:
    case os.name:
        pass

# Complex nesting
match {"type": "circle", "data": {"radius": 5, "center": (0, 0)}}:
    case {"type": "circle", "data": {"radius": r, "center": (x, y)}} if r > 0:
        pass
    case {"type": "rect", "data": {"width": w, "height": h}}:
        pass

# String literal in sequence
match ["hello", 42]:
    case [str(s), int(n)]:
        pass

# Negative literal pattern
match -1:
    case -1:
        pass
    case -3.14:
        pass

# match and case are soft keywords — can be used as identifiers
match = 1
case = 2
_ = match + case

# Star pattern in mapping
match {"a": 1, "b": 2, "c": 3}:
    case {"a": 1, **rest}:
        pass

# ═══════════════════════════════════════════════════════════════════════
# FUNCTIONS
# ═══════════════════════════════════════════════════════════════════════

# Basic
def simple(): pass

def with_return():
    return 42

def with_args(a, b, c):
    return a + b + c

# Default args
def defaults(a, b=10, c=20):
    pass

# *args, **kwargs
def variadic(*args, **kwargs):
    pass

# Keyword-only (after *)
def keyword_only(a, *, key1, key2="default"):
    pass

# Positional-only (3.8 — PEP 570)
def positional_only(a, b, /, c, d):
    pass

def pos_and_kw_only(a, b, /, c, *, d, e):
    pass

def only_positional(p1, p2, /):
    pass

def complex_sig(p1, p2, /, p_or_k1, p_or_k2=None, *, kw1, kw2=True):
    pass

# Annotations
def annotated(x: int, y: str = "hello") -> bool:
    return True

def complex_annotations(
    x: list[int],
    y: dict[str, list[tuple[int, ...]]],
    z: int | str | None,  # 3.10 union
) -> tuple[int, ...]:
    return (1,)

# Decorators
def my_decorator(func):
    return func

@my_decorator
def decorated(): pass

# Complex decorator expressions (3.9 — PEP 614)
decorators = [my_decorator]

@decorators[0]
def decorated2(): pass

@(lambda f: f)
def decorated3(): pass

@my_decorator
@my_decorator
def multi_decorated(): pass

# Lambda
fn = lambda: None
fn2 = lambda x: x + 1
fn3 = lambda x, y=10: x + y
fn4 = lambda *a, **k: (a, k)
fn5 = lambda x, /, y, *, z=1: x + y + z  # pos-only in lambda (3.8)
fn6 = lambda: (yield)  # legal in some contexts as generator expression

# Nested functions & closures
def outer():
    x = 10
    def inner():
        nonlocal x
        x += 1
        return x
    return inner

# Global / nonlocal
g = 0
def modify_global():
    global g
    g = 1

# Recursive
def factorial(n):
    return 1 if n <= 1 else n * factorial(n - 1)

# Docstrings
def with_docstring():
    """This is a docstring."""
    pass

def with_multiline_docstring():
    """
    This is a multiline
    docstring with multiple lines.

    Args:
        None
    Returns:
        None
    """
    pass

# Return annotations with complex types
def complex_return() -> dict[str, list[int | str]]:
    return {}

# Empty return
def empty_return():
    return

# Yield (generator)
def generator():
    yield

def generator_values():
    yield 1
    yield 2
    yield 3

def generator_expression_func():
    yield from range(10)

def generator_send():
    value = yield 42
    yield value

# Yield in complex contexts
def yield_in_loop():
    for i in range(10):
        x = yield i
        if x is not None:
            yield x * 2


# ═══════════════════════════════════════════════════════════════════════
# CLASSES
# ═══════════════════════════════════════════════════════════════════════

class Empty: pass

class WithDocstring:
    """A class with a docstring."""

class Basic:
    x = 10
    y: int = 20
    z: str

    def method(self):
        pass

    @staticmethod
    def static_method():
        pass

    @classmethod
    def class_method(cls):
        pass

    def __init__(self):
        self.instance_var = 1

    def __repr__(self):
        return "Basic()"

    def __str__(self):
        return "basic"

# Inheritance
class Child(Basic):
    pass

class MultiInherit(Basic, Empty):
    pass

# Metaclass & keywords
class Meta(type):
    pass

class WithMeta(metaclass=Meta):
    pass

class WithKeyword(Basic, metaclass=type, extra_kw=True):
    pass

# Properties
class WithProperties:
    def __init__(self):
        self._x = 0

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, value):
        self._x = value

    @x.deleter
    def x(self):
        del self._x

# Slots
class WithSlots:
    __slots__ = ('x', 'y')

class WithSlotsList:
    __slots__ = ['x', 'y']

# Class with __init_subclass__
class Plugin:
    def __init_subclass__(cls, /, name=None, **kwargs):
        super().__init_subclass__(**kwargs)
        cls.name = name

class MyPlugin(Plugin, name="test"):
    pass

# Abstract (just the syntax, no abc import needed for parsing)
class AbstractLike:
    def method(self):
        raise NotImplementedError

# Dunder methods
class DunderFest:
    def __init__(self): pass
    def __del__(self): pass
    def __repr__(self): pass
    def __str__(self): pass
    def __bytes__(self): pass
    def __format__(self, spec): pass
    def __lt__(self, other): pass
    def __le__(self, other): pass
    def __eq__(self, other): pass
    def __ne__(self, other): pass
    def __gt__(self, other): pass
    def __ge__(self, other): pass
    def __hash__(self): pass
    def __bool__(self): pass
    def __getattr__(self, name): pass
    def __setattr__(self, name, value): pass
    def __delattr__(self, name): pass
    def __dir__(self): pass
    def __get__(self, obj, objtype=None): pass
    def __set__(self, obj, value): pass
    def __delete__(self, obj): pass
    def __set_name__(self, owner, name): pass
    def __call__(self): pass
    def __len__(self): pass
    def __getitem__(self, key): pass
    def __setitem__(self, key, value): pass
    def __delitem__(self, key): pass
    def __iter__(self): pass
    def __next__(self): pass
    def __reversed__(self): pass
    def __contains__(self, item): pass
    def __add__(self, other): pass
    def __radd__(self, other): pass
    def __iadd__(self, other): pass
    def __mul__(self, other): pass
    def __matmul__(self, other): pass
    def __truediv__(self, other): pass
    def __floordiv__(self, other): pass
    def __mod__(self, other): pass
    def __pow__(self, other): pass
    def __and__(self, other): pass
    def __or__(self, other): pass
    def __xor__(self, other): pass
    def __lshift__(self, other): pass
    def __rshift__(self, other): pass
    def __neg__(self): pass
    def __pos__(self): pass
    def __abs__(self): pass
    def __invert__(self): pass
    def __int__(self): pass
    def __float__(self): pass
    def __complex__(self): pass
    def __index__(self): pass
    def __enter__(self): pass
    def __exit__(self, exc_type, exc_val, exc_tb): pass
    def __await__(self): pass
    def __aiter__(self): pass
    def __anext__(self): pass
    def __aenter__(self): pass
    def __aexit__(self, *args): pass
    def __class_getitem__(cls, item): pass
    def __buffer__(self, flags): pass       # 3.12
    def __release_buffer__(self, buf): pass  # 3.12

# Nested classes
class Outer:
    class Inner:
        class InnerInner:
            pass

# Dynamic bases with expressions
bases = (Basic,)
# class DynBases(*bases): pass  # star-unpack in bases — valid syntax


# ═══════════════════════════════════════════════════════════════════════
# TYPE PARAMETER SYNTAX (3.12 — PEP 695)
# ═══════════════════════════════════════════════════════════════════════

if False:
    # type statement (soft keyword)
    type Vector = list[float]
    type Point = tuple[int, int]
    type Callback[T] = Callable[[T], None]
    type Matrix[T] = list[list[T]]
    type RecursiveList[T] = T | list[RecursiveList[T]]

    # Generic function with new syntax
    def first[T](lst: list[T]) -> T:
        return lst[0]

    def pair[T, U](a: T, b: U) -> tuple[T, U]:
        return (a, b)

    # TypeVar with bound
    def process[T: int](x: T) -> T:
        return x

    # TypeVar with constraints
    def convert[T: (int, str)](x: T) -> T:
        return x

    # ParamSpec
    def decorator[**P, R](func: Callable[P, R]) -> Callable[P, R]:
        return func

    # TypeVarTuple
    def variadic_generic[*Ts](*args: *Ts) -> tuple[*Ts]:
        return args

    # Generic class with new syntax
    class Stack[T]:
        def __init__(self) -> None:
            self._items: list[T] = []
        def push(self, item: T) -> None:
            self._items.append(item)
        def pop(self) -> T:
            return self._items.pop()

    class Pair[T, U]:
        first: T
        second: U

    class MyDict[K: str, V](dict[K, V]):
        pass

    # Nested generics
    class Container[T]:
        class Inner[U]:
            def combine(self, t: T, u: U) -> tuple[T, U]:
                return (t, u)

    # TypeVar default values (3.13 — PEP 696)
    type DefaultType[T = int] = list[T]
    def with_default[T = str](x: T) -> T:
        return x
    class WithDefault[T = float]:
        value: T


# ═══════════════════════════════════════════════════════════════════════
# COMPREHENSIONS & GENERATOR EXPRESSIONS
# ═══════════════════════════════════════════════════════════════════════

# List comprehension
lc1 = [x for x in range(10)]
lc2 = [x for x in range(10) if x % 2 == 0]
lc3 = [x * y for x in range(5) for y in range(5)]
lc4 = [x for x in range(10) if x > 2 if x < 8]  # multiple ifs
lc5 = [[j for j in range(3)] for i in range(3)]  # nested

# Set comprehension
sc1 = {x for x in range(10)}
sc2 = {x for x in range(10) if x % 2}

# Dict comprehension
dc1 = {x: x**2 for x in range(10)}
dc2 = {k: v for k, v in zip("abc", [1, 2, 3])}
dc3 = {k: v for k, v in zip("abc", [1, 2, 3]) if v > 1}

# Generator expression
ge1 = (x for x in range(10))
ge2 = (x for x in range(10) if x % 2)
ge3 = sum(x for x in range(10))  # no extra parens needed in function call

# Nested comprehensions
nested = [(i, j, k) for i in range(3) for j in range(3) for k in range(3)]

# Walrus in comprehension (3.8)
walrus_comp = [y for x in range(10) if (y := x * 2) > 8]

# Async comprehensions (syntax only — inside async context)
async def async_comp_example():
    # These need async iterables at runtime, parser just needs the syntax
    async def arange(n):
        for i in range(n):
            yield i

    ac1 = [x async for x in arange(10)]
    ac2 = {x async for x in arange(10)}
    ac3 = {x: x async for x in arange(10)}
    ac4 = [x async for x in arange(10) if x > 5]
    ge_async = (x async for x in arange(10))
    ac5 = [x async for x in arange(10) if await something()]


# ═══════════════════════════════════════════════════════════════════════
# EXCEPTION HANDLING
# ═══════════════════════════════════════════════════════════════════════

# Basic try/except
try:
    pass
except:
    pass

try:
    pass
except Exception:
    pass

try:
    pass
except Exception as e:
    pass

try:
    pass
except (TypeError, ValueError):
    pass

try:
    pass
except (TypeError, ValueError) as e:
    pass

# try/except/else/finally
try:
    pass
except Exception:
    pass
else:
    pass
finally:
    pass

# try/finally (no except)
try:
    pass
finally:
    pass

# Raise
def raise_examples():
    raise
    raise ValueError
    raise ValueError("msg")
    raise ValueError("msg") from TypeError("cause")
    raise ValueError from None  # suppress context

# Nested try
try:
    try:
        pass
    except ValueError:
        pass
except TypeError:
    pass

# Exception group & except* (3.11 — PEP 654)
try:
    raise ExceptionGroup("group", [ValueError("a"), TypeError("b")])
except* ValueError as eg:
    pass
except* TypeError as eg:
    pass

try:
    pass
except* (ValueError, TypeError) as eg:
    pass

# Nested except*
try:
    pass
except* ValueError:
    try:
        pass
    except* TypeError:
        pass

# except* with ExceptionGroup
try:
    raise ExceptionGroup("g", [
        ValueError("v"),
        ExceptionGroup("nested", [TypeError("t"), KeyError("k")])
    ])
except* ValueError:
    pass
except* (TypeError, KeyError):
    pass


# ═══════════════════════════════════════════════════════════════════════
# CONTEXT MANAGERS
# ═══════════════════════════════════════════════════════════════════════

import tempfile, os

# Basic with
with tempfile.TemporaryFile() as f:
    pass

# Multiple context managers (3.1+)
with tempfile.TemporaryFile() as f1, tempfile.TemporaryFile() as f2:
    pass

# Parenthesized context managers (3.10 — PEP 617 new parser)
with (tempfile.TemporaryFile() as f1, tempfile.TemporaryFile() as f2):
    pass

with (
    tempfile.TemporaryFile() as f1,
    tempfile.TemporaryFile() as f2,
    tempfile.TemporaryFile() as f3,
):
    pass

# Without 'as'
with tempfile.TemporaryFile():
    pass

# Nested with
with tempfile.TemporaryFile() as f:
    with tempfile.TemporaryFile() as g:
        pass

# With expression (no variable capture)
with (tempfile.TemporaryFile(), tempfile.TemporaryFile()):
    pass


# ═══════════════════════════════════════════════════════════════════════
# ASYNC / AWAIT (comprehensive)
# ═══════════════════════════════════════════════════════════════════════

import asyncio

async def basic_coro():
    pass

async def with_await():
    await asyncio.sleep(0)

async def with_return():
    return 42

async def async_generator():
    yield 1
    yield 2
    await asyncio.sleep(0)
    yield 3

async def async_generator_with_return():
    yield 1
    return  # legal in async generators (no value though)

# Async for
async def async_for():
    async def aiter():
        for i in range(3):
            yield i
    async for item in aiter():
        pass
    else:
        pass  # async-for-else

# Async with
async def async_with():
    class ACM:
        async def __aenter__(self): return self
        async def __aexit__(self, *a): pass
    async with ACM() as ctx:
        pass

# Async with parenthesized (3.10)
async def async_with_paren():
    class ACM:
        async def __aenter__(self): return self
        async def __aexit__(self, *a): pass
    async with (ACM() as a, ACM() as b):
        pass

# Async comprehensions
async def all_async_comps():
    async def ag():
        for i in range(5): yield i
    a = [i async for i in ag()]
    b = {i async for i in ag()}
    c = {i: i async for i in ag()}
    d = [i async for i in ag() if i > 2]

# Nested async/sync
async def nested_async():
    def sync_inner():
        pass
    async def async_inner():
        await asyncio.sleep(0)
    sync_inner()
    await async_inner()

# Await in expressions
async def await_exprs():
    x = 1 + await asyncio.sleep(0, result=1)
    lst = [await asyncio.sleep(0, result=i) for i in range(3)]
    if await asyncio.sleep(0, result=True):
        pass
    while await asyncio.sleep(0, result=False):
        pass


# ═══════════════════════════════════════════════════════════════════════
# IMPORTS
# ═══════════════════════════════════════════════════════════════════════

import sys
import os.path
import os as operating_system
from os import path
from os import path as p
from os import (getcwd, listdir)
from os import (
    getcwd,
    listdir,
    stat,
)
from os.path import (
    join,
    dirname as dn,
    basename,
)
from sys import *  # star import

# Relative imports (valid syntax, would need package context)
if False:
    from . import sibling
    from .. import parent
    from .sibling import something
    from ...deep import module


# ═══════════════════════════════════════════════════════════════════════
# GLOBAL & NONLOCAL
# ═══════════════════════════════════════════════════════════════════════

global_var = 0

def globals_and_nonlocals():
    global global_var
    global_var = 1

    x = 10
    def inner():
        nonlocal x
        x = 20

    def multi_nonlocal():
        nonlocal x
        y = 30
        def innermost():
            nonlocal y
            y = 40


# ═══════════════════════════════════════════════════════════════════════
# ASSERT, DEL, PASS, ELLIPSIS
# ═══════════════════════════════════════════════════════════════════════

assert True
assert True, "message"
assert (1 == 1), f"expected equal"

del x
del a, b
obj = type('Obj', (), {'x': 1, 'y': 2})()
del obj.x

lst = [1, 2, 3]
del lst[0]
del lst[0:2]

pass
...  # Ellipsis as statement (valid in stubs, etc.)


# ═══════════════════════════════════════════════════════════════════════
# PRINT, EXEC, EVAL (as functions)
# ═══════════════════════════════════════════════════════════════════════

print()
print("hello")
print("hello", "world")
print("hello", end="")
print("hello", file=sys.stdout, flush=True)
print("a", "b", "c", sep=", ")

exec("x = 1")
exec("x = 1", {})
exec("x = 1", {}, {})

eval("1 + 2")
eval("1 + 2", {})
eval("1 + 2", {}, {})


# ═══════════════════════════════════════════════════════════════════════
# SLICING
# ═══════════════════════════════════════════════════════════════════════

lst = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

_ = lst[0]
_ = lst[-1]
_ = lst[1:5]
_ = lst[::2]
_ = lst[::-1]
_ = lst[1:8:2]
_ = lst[:5]
_ = lst[5:]
_ = lst[:]
_ = lst[1:2:3]

# Multi-dimensional slicing (for numpy-like objects)
class MultiDim:
    def __getitem__(self, key): return key

md = MultiDim()
_ = md[1, 2]
_ = md[1:2, 3:4]
_ = md[..., 1]
_ = md[:, :]
_ = md[0, ..., -1]
_ = md[1:2:3, 4:5:6, ...]

# Slice assignment
lst[0:2] = [10, 20]
lst[::2] = [0] * 5


# ═══════════════════════════════════════════════════════════════════════
# DATACLASSES, NAMEDTUPLE, SLOTS (as syntax patterns)
# ═══════════════════════════════════════════════════════════════════════

from dataclasses import dataclass, field, KW_ONLY
from typing import ClassVar

@dataclass
class SimpleData:
    x: int
    y: str = "default"

@dataclass(frozen=True)
class FrozenData:
    x: int
    y: int

@dataclass(slots=True)  # 3.10
class SlottedData:
    x: int
    y: int

@dataclass(kw_only=True)  # 3.10
class KWOnlyData:
    x: int
    y: int

@dataclass
class ComplexData:
    x: int
    y: str = "default"
    z: list[int] = field(default_factory=list)
    class_var: ClassVar[int] = 0
    _: KW_ONLY
    keyword_only_field: int = 10

@dataclass(match_args=True, slots=True, frozen=True)
class FullFeaturedData:
    x: int
    y: str
    z: float = 0.0

# Named tuples
from typing import NamedTuple
from collections import namedtuple

PointNT = namedtuple('PointNT', ['x', 'y'])
PointNT2 = namedtuple('PointNT2', 'x y z')

class TypedPoint(NamedTuple):
    x: int
    y: int
    z: float = 0.0


# ═══════════════════════════════════════════════════════════════════════
# TYPE ANNOTATIONS & TYPING MODULE (comprehensive)
# ═══════════════════════════════════════════════════════════════════════

from typing import (
    Any, Union, Optional, Final, Literal, TypeAlias,
    TypeGuard, TypeVar, ParamSpec, Concatenate,
    Protocol, runtime_checkable, overload,
    Generic, TypedDict, Never, Self,
    Unpack, TypeVarTuple, Required, NotRequired,
    LiteralString, assert_type, reveal_type,
    TYPE_CHECKING, final, no_type_check,
    get_type_hints, cast, dataclass_transform,
    override,  # 3.12
    TypeIs,    # 3.10+
)

# Basic annotations
x_int: int = 1
x_str: str = "hello"
x_float: float = 1.0
x_bool: bool = True
x_bytes: bytes = b"hi"
x_none: None = None
x_any: Any = "anything"

# Generic built-in types (3.9 — PEP 585)
x_list: list[int] = [1, 2, 3]
x_dict: dict[str, int] = {"a": 1}
x_tuple: tuple[int, str, float] = (1, "a", 1.0)
x_tuple_var: tuple[int, ...] = (1, 2, 3)
x_set: set[int] = {1, 2}
x_frozenset: frozenset[str] = frozenset({"a"})
x_type: type[int] = int

# Union types (3.10 — PEP 604)
x_union: int | str = 1
x_union2: int | str | None = None
x_union3: list[int | str] = [1, "a"]
x_optional: int | None = None  # modern Optional

# Older Union style (still valid)
x_old_union: Union[int, str] = 1
x_old_optional: Optional[int] = None

# Literal
x_lit: Literal["a", "b", "c"] = "a"
x_lit2: Literal[1, 2, 3] = 1
x_lit3: Literal[True] = True

# Final
x_final: Final[int] = 42
X_CONSTANT: Final = "immutable"

# TypeAlias (3.10)
Vector: TypeAlias = list[float]
Matrix: TypeAlias = list[list[float]]

# Callable types
from typing import Callable
x_callable: Callable[[int, str], bool] = lambda x, y: True
x_callable2: Callable[..., int] = lambda: 1
x_callable3: Callable[Concatenate[int, ParamSpec("P")], None] = None  # type: ignore

# TypeVar
T = TypeVar('T')
T_co = TypeVar('T_co', covariant=True)
T_contra = TypeVar('T_contra', contravariant=True)
T_bound = TypeVar('T_bound', bound=int)
T_constrained = TypeVar('T_constrained', int, str)

# ParamSpec
P = ParamSpec('P')

# TypeVarTuple (3.11)
Ts = TypeVarTuple('Ts')

# TypedDict
class Movie(TypedDict):
    name: str
    year: int

class PartialMovie(TypedDict, total=False):
    name: str
    year: int

class MixedMovie(TypedDict):
    name: Required[str]
    year: Required[int]
    rating: NotRequired[float]

# Protocol
@runtime_checkable
class Drawable(Protocol):
    def draw(self) -> None: ...

class SupportsClose(Protocol):
    def close(self) -> None: ...

# Generic protocol
class Container(Protocol[T]):
    def get(self) -> T: ...
    def set(self, value: T) -> None: ...

# Overloaded functions
@overload
def process(x: int) -> int: ...
@overload
def process(x: str) -> str: ...
def process(x):
    return x

# TypeGuard (3.10)
def is_str_list(val: list[object]) -> TypeGuard[list[str]]:
    return all(isinstance(x, str) for x in val)

# TypeIs (3.10+)
if False:
    def is_int(val: object) -> TypeIs[int]:
        return isinstance(val, int)

# Self (3.11)
class Builder:
    def set_x(self, x: int) -> Self:
        return self

# Never (3.11)
def never_returns() -> Never:
    raise RuntimeError

# LiteralString (3.11)
def safe_query(sql: LiteralString) -> None:
    pass

# @override (3.12)
class Parent:
    def method(self) -> None: pass

class ChildOverride(Parent):
    @override
    def method(self) -> None: pass

# @final
@final
class FinalClass: pass

class HasFinal:
    @final
    def method(self): pass

# @no_type_check
@no_type_check
def untyped(x, y, z):
    return x + y + z

# @dataclass_transform (3.11)
@dataclass_transform()
def my_dataclass(cls): return cls

# Complex nested annotations
x_complex: dict[str, list[tuple[int, Optional[str], set[frozenset[bytes]]]]] = {}
x_nested: Callable[[list[int]], dict[str, set[tuple[int, ...]]]] = lambda x: {}

# Forward references
class Tree:
    left: Tree | None = None
    right: Tree | None = None
    children: list[Tree] = []

# String annotations (always valid)
def forward_ref(x: "SomeClass") -> "SomeClass": ...  # type: ignore


# ═══════════════════════════════════════════════════════════════════════
# DECORATORS (all styles)
# ═══════════════════════════════════════════════════════════════════════

import functools

# Simple
@staticmethod
def s(): pass

# With arguments
@functools.lru_cache(maxsize=128)
def cached(n):
    return n

# Stacked
@staticmethod
@functools.lru_cache(maxsize=None)
def stacked():
    return 1

# Arbitrary expressions as decorators (3.9 — PEP 614)
d_list = [lambda f: f, lambda f: f]

@d_list[0]
def using_subscript(): pass

@(lambda f: f)
def using_lambda_dec(): pass

cond = True
@(my_decorator if cond else (lambda f: f))
def conditional_decorator(): pass

# Decorator accessing attribute
class DecoratorHolder:
    @staticmethod
    def dec(f): return f

@DecoratorHolder.dec
def from_class_dec(): pass

# Complex expression
@functools.reduce(lambda a, b: lambda f: a(b(f)), [my_decorator, my_decorator])
def reduced_decorators(): pass


# ═══════════════════════════════════════════════════════════════════════
# COLLECTION LITERALS & DISPLAYS
# ═══════════════════════════════════════════════════════════════════════

# Lists
l0 = []
l1 = [1]
l2 = [1, 2, 3]
l3 = [1, 2, 3,]  # trailing comma
l4 = [
    1,
    2,
    3,
]

# Tuples
t0 = ()
t1 = (1,)           # single-element tuple
t2 = (1, 2, 3)
t3 = 1, 2, 3        # tuple without parens
t4 = 1,              # single-element tuple without parens

# Sets
s0 = {1}
s1 = {1, 2, 3}
s2 = {1, 2, 3,}

# Dicts
d0 = {}
d1 = {"a": 1}
d2 = {"a": 1, "b": 2}
d3 = {"a": 1, "b": 2,}
d4 = {
    "a": 1,
    "b": 2,
    "c": 3,
}

# Nested
nested_lit = {
    "list": [1, [2, [3]]],
    "tuple": (1, (2, (3,))),
    "set": {1, 2, frozenset({3, 4})},
    "dict": {"inner": {"deep": True}},
}

# Empty set vs empty dict
empty_dict = {}
empty_set = set()


# ═══════════════════════════════════════════════════════════════════════
# CONDITIONAL IMPORTS & __all__
# ═══════════════════════════════════════════════════════════════════════

if TYPE_CHECKING:
    from typing import Sequence, Mapping
    from collections.abc import Iterator

__all__ = [
    "Simple",
    "Basic",
    "Point",
]

# Conditional import with try/except
try:
    import ujson as json
except ImportError:
    import json  # type: ignore


# ═══════════════════════════════════════════════════════════════════════
# STRING METHODS AS EXPRESSIONS & CHAINING
# ═══════════════════════════════════════════════════════════════════════

_ = "hello".upper()
_ = "  hello  ".strip().lower().replace("hello", "world")
_ = ",".join(["a", "b", "c"])
_ = "hello world".split()
_ = "hello world".split(" ", maxsplit=1)
_ = (
    "hello"
    .upper()
    .replace("H", "J")
    .strip()
)


# ═══════════════════════════════════════════════════════════════════════
# COMPLEX EXPRESSIONS & EDGE CASES
# ═══════════════════════════════════════════════════════════════════════

# Parenthesized expressions
_ = (1)
_ = (1 + 2) * 3
_ = ((((1))))

# Nested ternary
_ = 1 if True else 2 if False else 3

# Chained function calls
def identity(x): return x
_ = identity(identity(identity(42)))

# Attribute chains
_ = "hello".__class__.__name__.__class__

# Subscript chains
d = {"a": {"b": {"c": 1}}}
_ = d["a"]["b"]["c"]

# Mixed chains
_ = [1, 2, 3].__class__.__name__[0]

# Star expressions in returns
def return_star():
    return *[1, 2], 3

# Yield expressions
def complex_yield():
    x = (yield 1)
    y = (yield from range(10))

# Lambda edge cases
_ = (lambda: (lambda: (lambda: 42)())())()
_ = lambda *a, **k: (*a, k)

# Multiple unary operators
_ = --1
_ = ~~0
_ = not not True
_ = -+-+1

# Long expression
_ = (1 + 2 * 3 - 4 / 5 // 6 % 7 ** 8
     & 0xFF | 0x00 ^ 0x0F << 2 >> 1)

# Conditional with complex sub-expressions
_ = [x for x in range(10)] if True else {x: x for x in range(10)}

# Semicolons (multiple statements per line)
a = 1; b = 2; c = 3

# Backslash line continuation
_ = 1 + \
    2 + \
    3

# Implicit line continuation
_ = (
    1 +
    2 +
    3
)
_ = [
    1,
    2,
    3,
]
_ = {
    "a": 1,
    "b": 2,
}


# ═══════════════════════════════════════════════════════════════════════
# TYPE COMMENTS (still parseable, legacy)
# ═══════════════════════════════════════════════════════════════════════

x = 1  # type: int
y = {}  # type: dict[str, int]
def legacy_typed(x, y):  # type: (int, str) -> bool
    return True


# ═══════════════════════════════════════════════════════════════════════
# SOFT KEYWORDS (context-dependent keywords)
# ═══════════════════════════════════════════════════════════════════════

# match, case, type, _ are soft keywords — valid as identifiers
match = 1
case = 2
type = 3
_ = match + case + type

def match(case):
    return case

class type:
    case = 1

# Using soft keywords as variables then in their keyword context
match_val = 42
match match_val:
    case 42:
        pass

# Restore type
type = __builtins__.__dict__['type'] if isinstance(__builtins__, dict) else __builtins__.type  # noqa


# ═══════════════════════════════════════════════════════════════════════
# CLOSURES, SCOPING, CELL VARIABLES
# ═══════════════════════════════════════════════════════════════════════

def closure_factory():
    captured = []
    def append(x):
        captured.append(x)
    def get():
        return captured
    return append, get

def complex_scoping():
    x = 1
    def level1():
        y = 2
        def level2():
            z = 3
            def level3():
                nonlocal z
                z = 30
                return x + y + z
            return level3
        return level2
    return level1


# ═══════════════════════════════════════════════════════════════════════
# DESCRIPTORS, __slots__, CLASS BODY ODDITIES
# ═══════════════════════════════════════════════════════════════════════

class Descriptor:
    def __set_name__(self, owner, name):
        self.name = name
    def __get__(self, obj, objtype=None):
        if obj is None: return self
        return getattr(obj, f'_{self.name}', None)
    def __set__(self, obj, value):
        setattr(obj, f'_{self.name}', value)
    def __delete__(self, obj):
        delattr(obj, f'_{self.name}')

class UsingDescriptor:
    attr = Descriptor()

# Class body with complex statements
class ClassBody:
    for i in range(3):
        locals()[f'attr_{i}'] = i

    if True:
        conditional_attr = True
    else:
        conditional_attr = False

    try:
        risky = 1 / 1
    except ZeroDivisionError:
        risky = 0

    [x for x in range(0)]  # comprehension in class body


# ═══════════════════════════════════════════════════════════════════════
# METACLASSES & __init_subclass__
# ═══════════════════════════════════════════════════════════════════════

class MetaExample(type):
    def __new__(mcs, name, bases, namespace, **kwargs):
        return super().__new__(mcs, name, bases, namespace)
    def __init__(cls, name, bases, namespace, **kwargs):
        super().__init__(name, bases, namespace)
    def __prepare__(metacls, name, bases, **kwargs):
        return {}

class WithMeta2(metaclass=MetaExample, custom_kwarg=True):
    pass

class SubclassHook:
    def __init_subclass__(cls, /, flavor="vanilla", **kwargs):
        super().__init_subclass__(**kwargs)
        cls.flavor = flavor

class Chocolate(SubclassHook, flavor="chocolate"):
    pass


# ═══════════════════════════════════════════════════════════════════════
# MAGIC / DUNDER USAGE IN EXPRESSIONS
# ═══════════════════════════════════════════════════════════════════════

_ = (1).__add__(2)
_ = "hello".__len__()
_ = [1, 2, 3].__iter__().__next__()
_ = {"a": 1}.__contains__("a")
_ = (1).__class__
_ = int.__mro__


# ═══════════════════════════════════════════════════════════════════════
# MULTILINE CONSTRUCTS & CONTINUATION
# ═══════════════════════════════════════════════════════════════════════

# Implicit continuation in brackets
result = (
    1
    + 2
    + 3
)

items = [
    "a",
    "b",
    "c",
]

mapping = {
    "key1": "value1",
    "key2": "value2",
}

call_result = dict(
    a=1,
    b=2,
    c=3,
)

# Backslash continuation
long_string = "hello " \
              "world " \
              "foo"

long_condition = (True
                  and False
                  or True)

# Multi-line function definition
def multi_line_func(
    arg1: int,
    arg2: str,
    arg3: float = 1.0,
    *args: int,
    **kwargs: str,
) -> None:
    pass

# Multi-line class definition
class MultiLineClass(
    Basic,
    Empty,
    metaclass=type,
):
    pass

# Multi-line import
from os.path import (
    join,
    dirname,
    basename,
    exists,
    isfile,
    isdir,
)

# Multi-line decorator with complex expression
@functools.lru_cache(
    maxsize=256,
    typed=True,
)
def cached_func(x):
    return x

# Multi-line dict comprehension
_ = {
    k: v
    for k in range(10)
    for v in range(10)
    if k == v
}


# ═══════════════════════════════════════════════════════════════════════
# ASSIGNMENT EXPRESSIONS IN VARIOUS CONTEXTS
# ═══════════════════════════════════════════════════════════════════════

# In assert
assert (x := 10) > 5

# In return
def walrus_return():
    return (x := 42)

# In default argument (weird but valid at parse level)
# def f(x=(y := 10)): pass  # Actually disallowed — SyntaxError

# Deeply nested
if (a := (b := (c := 1) + 1) + 1):
    pass


# ═══════════════════════════════════════════════════════════════════════
# WEIRD BUT LEGAL SYNTAX
# ═══════════════════════════════════════════════════════════════════════

# Empty containers as conditions
if []:
    pass
if {}:
    pass
if ():
    pass
if set():
    pass

# Chained comparisons with 'is' and 'in'
_ = 1 < 2 < 3
_ = "a" in "abc" in ["abc"]  # chained, though weird

# Multiple stars in tuple/list construction
_ = (*range(3), *range(3, 6))
_ = [*range(3), *"abc", *b"def"]

# Nested subscript
_ = {0: {1: {2: "deep"}}}[0][1][2]

# Chained assignment with complex LHS
class ChainTest:
    x = y = [1, 2, 3]

# Single expression on a line (expression statements)
1
"string"
...
True
None

# Tuple expression statement
1, 2, 3

# Parenthesized assignment target
(x) = 1
((x)) = 1

# Empty function bodies with different fillers
def f1(): pass
def f2(): ...
def f3(): return
def f4():
    """Just a docstring."""

# Class with only ellipsis
class Stub: ...

# Class with only pass
class Stub2: pass

# Nested empty
class Outer2:
    class Inner2:
        def method(self):
            ...

# Very long default argument expression
def long_default(
    x=list(range(100)),
    y={str(i): i for i in range(50)},
    z=tuple(i**2 for i in range(20)),
):
    pass

# Star in for loop
for a, *b in [(1, 2, 3), (4, 5, 6)]:
    pass

for (a, b), c in [((1, 2), 3), ((4, 5), 6)]:
    pass

# Walrus in various positions
while data := [1, 2, 3]:
    break

# Parenthesized starred assignment
(*x,) = [1, 2, 3]
[*x] = [1, 2, 3]

# Assignment to attribute
class MutableObj:
    pass
obj2 = MutableObj()
obj2.x = 1
obj2.x += 1

# Assignment to subscript
d = {}
d["key"] = "value"
d["key"] += "!"

# Augmented assignment to subscript / attribute
lst = [0]
lst[0] += 1
lst[0] *= 2
lst[0] //= 1

# Delete from subscript and slice
lst2 = [1, 2, 3, 4, 5]
del lst2[0]
del lst2[1:3]
del lst2[::2]

# Comma-separated del
a, b, c = 1, 2, 3
del a, b, c


# ═══════════════════════════════════════════════════════════════════════
# ANNOTATIONS EVERYWHERE
# ═══════════════════════════════════════════════════════════════════════

# Variable annotations (module level)
module_var: int
module_var2: int = 42
module_var3: list[dict[str, tuple[int, ...]]] = []
module_var4: "ForwardRef" = None  # type: ignore

# Annotated assignments in different contexts
class AnnotatedClass:
    class_var: int = 0
    instance_var: str
    complex_var: dict[str, list[int | None]] = {}

    def __init__(self):
        self.x: int = 1
        self.y: str = "hello"

# Annotated assignment to complex target
# (a): int  = 1  # Not legal — annotation on parenthesized target is SyntaxError
# But this is fine:
a: int
a = 1


# ═══════════════════════════════════════════════════════════════════════
# ASYNC GENERATORS & ASYNC COMPREHENSIONS (more edge cases)
# ═══════════════════════════════════════════════════════════════════════

async def async_gen_edges():
    # yield in try/except/finally
    try:
        yield 1
    except Exception:
        yield 2
    finally:
        yield 3  # actually a SyntaxError at runtime for async gen, but parse-level OK

    # yield in with
    async def inner():
        class CM:
            async def __aenter__(self): return self
            async def __aexit__(self, *a): pass
        async with CM():
            yield 42

    # send values
    value = yield 10
    yield value

    # Async generator with walrus
    data = [x async for x in inner() if (y := x) > 0]


# ═══════════════════════════════════════════════════════════════════════
# RAISE FROM, EXCEPTION CHAINING
# ═══════════════════════════════════════════════════════════════════════

def exception_chaining():
    try:
        try:
            raise ValueError("original")
        except ValueError as e:
            raise TypeError("wrapper") from e

    except TypeError:
        pass

    # Suppress context
    try:
        raise RuntimeError("no context") from None
    except RuntimeError:
        pass

    # Bare raise
    try:
        try:
            raise ValueError
        except:
            raise  # re-raise
    except ValueError:
        pass


# ═══════════════════════════════════════════════════════════════════════
# COMPLEX DECORATOR PATTERNS
# ═══════════════════════════════════════════════════════════════════════

def parametrized_decorator(arg1, arg2=None):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        return wrapper
    return decorator

@parametrized_decorator("test", arg2="value")
def decorated_func():
    pass

# Decorator factory returning decorator
def make_retry(times=3):
    def decorator(func):
        def wrapper(*args, **kwargs):
            for _ in range(times):
                try:
                    return func(*args, **kwargs)
                except Exception:
                    pass
            raise RuntimeError("all retries failed")
        return wrapper
    return decorator

@make_retry(times=5)
def unreliable():
    pass


# ═══════════════════════════════════════════════════════════════════════
# MULTIPLE INHERITANCE & MRO EDGE CASES
# ═══════════════════════════════════════════════════════════════════════

class A: pass
class B(A): pass
class C(A): pass
class D(B, C): pass  # diamond inheritance

class Mixin1:
    def method(self): pass

class Mixin2:
    def method(self): pass

class Combined(Mixin1, Mixin2):
    def method(self):
        super().method()

# Super with arguments
class ExplicitSuper(Basic):
    def method(self):
        super(ExplicitSuper, self).method()


# ═══════════════════════════════════════════════════════════════════════
# SPECIAL SYNTAX FORMS
# ═══════════════════════════════════════════════════════════════════════

# Conditional import
import sys
if sys.platform == "win32":
    pass
elif sys.platform == "darwin":
    pass
else:
    pass

# Try import
try:
    import nonexistent_module  # type: ignore
except ModuleNotFoundError:
    nonexistent_module = None  # type: ignore

# __name__ guard
if __name__ == "__main__":
    pass

# __debug__ usage
if __debug__:
    pass

# Assertions with __debug__
assert __debug__ or True

# Empty module-level expressions
42
"module docstring literal"
...

# Type comment on assignment
x = []  # type: list[int]

# Semicolons
a = 1; b = 2; c = a + b

# Trailing comma in function call
print("a", "b", "c",)

# Trailing comma in function def
def trailing(a, b, c,):
    pass

# Trailing comma in class bases
class TrailingBases(A, B,):
    pass

# Mixed positional-only, keyword-only with defaults
def mixed_params(a, b=1, /, c=2, *args, d=3, **kwargs):
    pass


# ═══════════════════════════════════════════════════════════════════════
# PARENTHESIZED CONTEXT MANAGERS (3.10)
# ═══════════════════════════════════════════════════════════════════════

# Already covered above, but more edge cases
import contextlib

@contextlib.contextmanager
def dummy_cm():
    yield

with (dummy_cm()):
    pass

with (dummy_cm() as x):
    pass

with (dummy_cm() as x, dummy_cm() as y):
    pass

with (
    dummy_cm() as a,
    dummy_cm() as b,
    dummy_cm() as c,
):
    pass


# ═══════════════════════════════════════════════════════════════════════
# PEP 646 — TypeVarTuple & Unpack (3.11)
# ═══════════════════════════════════════════════════════════════════════

# Using typing module approach (works for parser testing)
from typing import TypeVarTuple, Unpack

Ts2 = TypeVarTuple('Ts2')

class Array(Generic[Unpack[Ts2]]):
    pass

def variadic_func(*args: Unpack[Ts2]) -> None:
    pass

# TypedDict with Unpack for **kwargs typing
class Options(TypedDict):
    timeout: int
    retries: int

def with_options(**kwargs: Unpack[Options]) -> None:
    pass


# ═══════════════════════════════════════════════════════════════════════
# WALRUS EVERYWHERE (edge-case positions)
# ═══════════════════════════════════════════════════════════════════════

# In dictionary literal
d = {(k := "key"): (v := "value")}

# In set literal
s = {(x := 1), (y := 2)}

# In list literal
l = [(a := 1), (b := 2), a + b]

# In argument
def takes_arg(x): return x
result = takes_arg(x := 42)

# In subscript
lst = [10, 20, 30]
_ = lst[(idx := 1)]

# In comparison
if (n := 10) > 5 and (m := n * 2) > 15:
    pass

# In ternary
_ = (x := 1) if True else (y := 2)

# Chained in comprehension
result2 = [(a, b) for x in range(5) if (a := x * 2) > 3 if (b := a + 1) < 10]


# ═══════════════════════════════════════════════════════════════════════
# EXCEPTION GROUP NESTING & except* EDGE CASES (3.11)
# ═══════════════════════════════════════════════════════════════════════

# except* cannot mix with except — but multiple except* are fine
try:
    pass
except* ValueError:
    pass
except* (TypeError, KeyError):
    pass
except* RuntimeError as eg:
    pass

# BaseExceptionGroup
try:
    raise BaseExceptionGroup("base", [KeyboardInterrupt()])
except* KeyboardInterrupt:
    pass

# Nested ExceptionGroup creation
def create_nested_eg():
    return ExceptionGroup("outer", [
        ValueError("v"),
        ExceptionGroup("inner", [
            TypeError("t"),
            ExceptionGroup("deep", [
                KeyError("k"),
            ]),
        ]),
    ])


# ═══════════════════════════════════════════════════════════════════════
# COMPLEX MATCH PATTERNS (more 3.10+ edge cases)
# ═══════════════════════════════════════════════════════════════════════

# Singleton patterns
match None:
    case None:
        pass

match True:
    case True:
        pass
    case False:
        pass

# Complex class pattern
from collections import namedtuple
Coord = namedtuple("Coord", ["x", "y", "z"])

match Coord(1, 2, 3):
    case Coord(x=1, y=2, z=z_val):
        pass
    case Coord(0, 0, 0):
        pass

# Nested OR patterns
match (1, "a"):
    case (1 | 2, "a" | "b"):
        pass

# Sequence with star in middle
match [1, 2, 3, 4, 5]:
    case [1, *middle, 5]:
        pass
    case [first, *_, last]:
        pass

# Guard with complex expression
match {"x": 10}:
    case {"x": x} if x > 0 and isinstance(x, int) and x < 100:
        pass

# Empty sequence and mapping
match []:
    case []:
        pass

match {}:
    case {}:
        pass

# Key expressions in mapping patterns (only literals and dotted names)
import http.client
match {"status": 404}:
    case {"status": 200}:
        pass
    case {"status": 404}:
        pass


# ═══════════════════════════════════════════════════════════════════════
# BYTES STRINGS EDGE CASES
# ═══════════════════════════════════════════════════════════════════════

# All prefix combinations
_ = b"bytes"
_ = B"bytes"
_ = rb"raw bytes"
_ = Rb"raw bytes"
_ = rB"raw bytes"
_ = RB"raw bytes"
_ = br"raw bytes"
_ = bR"raw bytes"
_ = Br"raw bytes"
_ = BR"raw bytes"

# Triple-quoted bytes
_ = b"""triple bytes"""
_ = b'''triple bytes'''
_ = rb"""raw triple"""
_ = rb'''raw triple'''

# Bytes with all escape types
_ = b"\x00\x01\xFF"
_ = b"\n\r\t\\\'\""
_ = b"\0"
_ = b"\a\b\f\v"
_ = b"\110\145\154\154\157"  # octal


# ═══════════════════════════════════════════════════════════════════════
# STRING PREFIX COMBINATIONS
# ═══════════════════════════════════════════════════════════════════════

_ = "normal"
_ = r"raw"
_ = R"Raw"
_ = f"f-string"
_ = F"F-string"
_ = rf"raw f"
_ = Rf"Raw f"
_ = rF"raw F"
_ = RF"Raw F"
_ = fr"f raw"
_ = Fr"F raw"
_ = fR"f Raw"
_ = FR"F Raw"
_ = u"unicode"
_ = U"Unicode"

# Triple-quoted variants
_ = f"""triple f"""
_ = f'''triple f'''
_ = rf"""raw triple f"""
_ = rf'''raw triple f'''


# ═══════════════════════════════════════════════════════════════════════
# COMPLEX LAMBDA EXPRESSIONS
# ═══════════════════════════════════════════════════════════════════════

# Lambda with all parameter types
_ = lambda a, b, c: a + b + c
_ = lambda a, b=1, c=2: a + b + c
_ = lambda *a: a
_ = lambda **k: k
_ = lambda *a, **k: (a, k)
_ = lambda a, /, b, *, c: a + b + c
_ = lambda a, b=1, /, c=2, *args, d=3, **kwargs: None

# Lambda returning tuple (no parens needed inside lambda body)
_ = lambda: 1, 2  # This is (lambda: 1), 2 — careful!
_ = lambda: (1, 2)  # This returns a tuple

# Lambda in comprehension
_ = [lambda x: x + i for i in range(5)]
_ = {i: lambda x, i=i: x + i for i in range(5)}

# Nested lambdas
_ = lambda f: lambda x: f(x)
_ = lambda: lambda: lambda: 42

# Lambda as default
def f(callback=lambda x: x):
    pass

# Lambda as dict value
dispatch = {
    "add": lambda x, y: x + y,
    "sub": lambda x, y: x - y,
    "mul": lambda x, y: x * y,
}

# Immediately invoked
_ = (lambda x: x * 2)(21)


# ═══════════════════════════════════════════════════════════════════════
# YIELD & YIELD FROM EDGE CASES
# ═══════════════════════════════════════════════════════════════════════

def gen_edges():
    # yield as expression
    x = yield
    x = yield 42

    # yield in ternary
    x = (yield 1) if True else (yield 2)

    # yield in boolean
    x = (yield 1) or (yield 2)
    x = (yield 1) and (yield 2)

    # yield from
    yield from range(10)
    yield from (x**2 for x in range(10))
    yield from []

    # yield from in assignment
    result = yield from iter([1, 2, 3])

    # yield in try
    try:
        yield 1
    except GeneratorExit:
        pass
    finally:
        pass

    # Multiple yields
    yield 1; yield 2; yield 3


# ═══════════════════════════════════════════════════════════════════════
# COMPLEX CLASS PATTERNS
# ═══════════════════════════════════════════════════════════════════════

# Class with complex class body
class Complex:
    # Class-level assignments
    X = 1
    Y: int = 2
    Z: list[int] = [1, 2, 3]

    # Class-level comprehension
    EVENS = [x for x in range(20) if x % 2 == 0]

    # Class-level conditional
    DEBUG = True
    if DEBUG:
        LOG_LEVEL = "DEBUG"
    else:
        LOG_LEVEL = "INFO"

    # Class-level try
    try:
        import json
        PARSER = json
    except ImportError:
        PARSER = None

    # Class-level for (populating class dict)
    for _i in range(5):
        locals()[f'field_{_i}'] = _i

    # Nested function in class body
    def _helper():
        return 42
    CONSTANT = _helper()
    del _helper  # cleanup

    # Class-level walrus
    if (threshold := 100) > 50:
        THRESHOLD = threshold

    # Methods with various signatures
    def regular(self): pass
    @staticmethod
    def static_m(): pass
    @classmethod
    def class_m(cls): pass

    async def async_method(self):
        pass

    # Property with all three
    @property
    def prop(self): return self.X
    @prop.setter
    def prop(self, v): self.X = v
    @prop.deleter
    def prop(self): del self.X


# ═══════════════════════════════════════════════════════════════════════
# MISC PYTHON 3.12 FEATURES
# ═══════════════════════════════════════════════════════════════════════

# Better error messages (no syntax impact, but worth noting)

# Improved NamedTuple / TypedDict — already covered

# Comprehension inlining (PEP 709) — no syntax change, but comprehension
# variables now "leak" less. Parser doesn't care.

# Per-interpreter GIL (3.12) — no syntax impact

# Immortal objects (3.12) — no syntax impact


# ═══════════════════════════════════════════════════════════════════════
# PYTHON 3.13 FEATURES
# ═══════════════════════════════════════════════════════════════════════

# JIT compiler (no syntax impact)
# Improved interactive interpreter (no syntax impact)
# Deprecation of some legacy features

# Improved error messages for:
# - Missing 'self' in methods
# - Incorrect use of 'is' with literals
# (These are warnings, not syntax changes)

# typing.TypeVar defaults (PEP 696) — covered in section 11
# typing.ReadOnly for TypedDict (3.13)
if False:
    from typing import ReadOnly
    class ReadOnlyTD(TypedDict):
        name: ReadOnly[str]
        age: int


# ═══════════════════════════════════════════════════════════════════════
# PYTHON 3.14 FEATURES
# ═══════════════════════════════════════════════════════════════════════

# --- Template Strings (PEP 750) ---
# t-strings — covered in section 2b

# --- Deferred Evaluation of Annotations (PEP 649) ---
# `from __future__ import annotations` behavior becomes default in 3.14.
# Forward references work without quotes.
# No new syntax per se, but annotation evaluation changes semantics.

# Example that benefits from PEP 649: mutual references
class Node:
    value: int
    children: list[Node]  # This "just works" with PEP 649

class LinkedNode:
    data: int
    next: LinkedNode | None = None  # Works without quotes in 3.14

# --- PEP 758: except without parentheses for multiple exceptions (3.14) ---
if False:
    try:
        pass
    except ValueError, TypeError:  # No parens needed in 3.14
        pass

    try:
        pass
    except ValueError, TypeError as e:
        pass

    try:
        pass
    except ValueError, TypeError, KeyError:
        pass

# --- PEP 761: deprecation of old-style formatting features (3.14) ---
# No syntax change

# --- Improved REPL and error messages ---
# No syntax impact


# ═══════════════════════════════════════════════════════════════════════
# REMAINING EDGE CASES & CORNER SYNTAX
# ═══════════════════════════════════════════════════════════════════════

# Empty match
match 42:
    case _:
        pass

# Semicolons after compound statements (legal oddity)
if True: pass; pass

# Nested match
match (1, 2):
    case (x, y):
        match x:
            case 1:
                pass

# Deeply nested control flow
for i in range(3):
    if i > 0:
        while i > 0:
            i -= 1
            if i == 1:
                for j in range(2):
                    try:
                        pass
                    except:
                        break
                continue
            break

# Expressions as statements
object()
type.__mro__
(lambda: None)()
[].append
{}.keys

# Call with generator expression as sole argument
_ = list(x for x in range(10))
_ = dict((k, v) for k, v in zip("abc", [1, 2, 3]))

# Starred assignment in for
for first, *rest in [(1, 2, 3), (4, 5, 6)]:
    pass

# Complex default values
def complex_defaults(
    a=[1, 2, 3],
    b={"key": "value"},
    c=(1, 2),
    d={1, 2, 3},
    e=lambda: 42,
    f=...,
    g=None,
    h=True,
    i=b"bytes",
):
    pass

# Unicode identifiers (more)
δ = 0.001
Δ = 1.0
α = β = γ = 0.0
сумма = 0  # Cyrillic
名前 = "name"  # Japanese
이름 = "name"  # Korean

# Function annotations with complex expressions
def crazy_annotations(
    x: int if True else str,  # type: ignore
    y: [int, str][0],
    z: (lambda: int)(),
) -> {True: int, False: str}[True]:
    pass

# Nested string quotes
_ = "he said \"hello\""
_ = 'she said \'hi\''
_ = """he said "hello" and she said 'hi'"""
_ = '''he said "hello" and she said 'hi' '''

# Empty f-string
_ = f""
_ = f''
_ = f""""""
_ = f''''''

# Line continuation in string
_ = "hello \
world"
_ = 'hello \
world'

# Null bytes in strings (via escape)
_ = "\x00"
_ = b"\x00"

# Very long number
_ = 1_000_000_000_000_000_000_000_000_000_000_000_000_000_000

# Chained comparison with all operators
_ = 1 <= 1 == 1 >= 1

# Power operator precedence
_ = -2**2  # = -(2**2) = -4
_ = 2**-1  # = 0.5
_ = 2**2**3  # = 2**(2**3) = 256 (right-associative)

# Mixed operations with parens
_ = (1 + 2) * (3 - 4) / (5 % 6) // (7 ** 8)

# Empty tuple in various contexts
_ = ()
_ = ((),)
_ = ((), ())
def returns_empty() -> tuple[()]:  # type: ignore
    return ()

# Starred in return and yield
def star_return():
    return *[1, 2], 3

def star_yield():
    yield *[1, 2], 3  # Actually a SyntaxError, but some parsers handle it

# __all__ with different types
__all__ = ("Simple", "Basic")
__all__ = ["Simple", "Basic"]
__all__ += ["Point"]

# Assignment expression type annotation interaction
# (walrus doesn't support annotations)
# (x := 1): int  # SyntaxError — just noting it here


# ═══════════════════════════════════════════════════════════════════════
# TRAILING COMMAS EVERYWHERE
# ═══════════════════════════════════════════════════════════════════════

# Function args
def tc(a, b, c,): pass
tc(1, 2, 3,)

# Lambda (trailing comma NOT allowed after last param before colon)
# _ = lambda a, b,: None  # SyntaxError

# But trailing comma in call to lambda
_ = (lambda a, b: None)(1, 2,)

# Collection literals
_ = [1, 2,]
_ = (1, 2,)
_ = {1, 2,}
_ = {"a": 1, "b": 2,}
_ = [
    1,
    2,
    3,
]

# Import
from os import (
    path,
    getcwd,
)

# Class bases
class TC(Basic, Empty,): pass

# Decorator args
@functools.lru_cache(maxsize=128,)
def tc_cached(x): return x

# Comprehension (no trailing comma in for clause — illegal)
# But in the iterable expression it's fine
_ = [x for x in (1, 2, 3,)]


# ═══════════════════════════════════════════════════════════════════════
# NUMERIC LITERAL EDGE CASES
# ═══════════════════════════════════════════════════════════════════════

# Leading/trailing underscores in numeric parts (NOT allowed, just testing valid ones)
_ = 1_0
_ = 1_0_0
_ = 0x_FF
_ = 0o_77
_ = 0b_1010
_ = 1_0.0_1
_ = 1_0e1_0
_ = 1_0j
_ = .1_2
_ = 1_2.
_ = 0.0_0_0_1

# Hex, oct, bin with underscores
_ = 0xFF_FF_FF_FF
_ = 0o77_77_77
_ = 0b1010_1010_1010


# ═══════════════════════════════════════════════════════════════════════
# MISCELLANEOUS STATEMENT FORMS
# ═══════════════════════════════════════════════════════════════════════

# Expression statements with side effects
print("hello")
[].append(1)
{}.update(a=1)

# Multiple assignment targets
a = b = c = d = e = 0

# Augmented assignment with complex LHS
class AugTarget:
    x = [0]
at = AugTarget()
at.x[0] += 1

# Chained attribute assignment
class Chain:
    class Inner:
        val = 0
ch = Chain()
ch.Inner.val = 42

# Delete various targets
x = y = z = 0
del x
lst = [1, 2, 3]
del lst[0]
del lst[:]
obj3 = type("Obj", (), {"a": 1})()
del obj3.a

# Multiple statements on one line with semicolons
a = 1; b = 2; c = a + b; print(c)

# Empty lines between statements (just whitespace — no syntax, but parsers must handle)


# (intentional blank lines above)

# Comments between continuation lines
_ = (1 +  # first
     2 +  # second
     3)   # third

# Nested function definitions at various depths
def depth0():
    def depth1():
        def depth2():
            def depth3():
                def depth4():
                    return "deep"
                return depth4
            return depth3
        return depth2
    return depth1


# ═══════════════════════════════════════════════════════════════════════
# FINAL SYNTAX CONSTRUCTS
# ═══════════════════════════════════════════════════════════════════════

# Ellipsis in slices
class EllipsisSlice:
    def __getitem__(self, key): return key
es = EllipsisSlice()
_ = es[...]
_ = es[..., 1]
_ = es[1, ..., 2]

# Complex dict unpacking
def merge_dicts():
    a = {"x": 1}
    b = {"y": 2}
    c = {"z": 3}
    return {**a, **b, **c, "w": 4}

# Complex set operations in expressions
_ = {1, 2, 3} | {4, 5} & {5, 6} - {6, 7} ^ {7, 8}

# Tuple as dictionary key
d = {(1, 2): "tuple key", (3,): "single", (): "empty"}

# Frozenset in set
_ = {frozenset({1, 2}), frozenset({3, 4})}

# Boolean as integer
_ = True + True  # = 2
_ = False * 100  # = 0
_ = True << 10   # = 1024

# None comparisons
_ = None is None
_ = None is not None

# String multiplication
_ = "ha" * 3
_ = 3 * "ha"

# List multiplication
_ = [0] * 10
_ = 10 * [None]

# Conditional import pattern
try:
    from collections import OrderedDict
except ImportError:
    from collections import OrderedDict  # same, just pattern

# Nested with and for
with open(os.devnull) as f:
    for line in f:
        pass

# Complex boolean expressions
_ = (True and False or True and not False or (True if False else True))

# Nested subscript with calls
class Builder2:
    def __getitem__(self, key):
        return self
    def __call__(self, *args):
        return self
b2 = Builder2()
_ = b2[0](1)[2](3, 4)[5]


# ═══════════════════════════════════════════════════════════════════════
# ENCODINGS & SPECIAL CHARACTERS IN IDENTIFIERS
# ═══════════════════════════════════════════════════════════════════════

# PEP 3131 — non-ASCII identifiers
résumé = "CV"
naïve = True
über = "over"
Ω = 6.28
ℕ = {0, 1, 2, 3}
is_prime = lambda n: n > 1
café_count = 42

# Emoji are NOT valid identifiers, but various Unicode categories are:
# Letters, combining marks, digits, connectors, etc.
_leading_underscore = 1
__double_leading = 2
__dunder__ = 3
x1234 = 4
αβγδ = "greek"
АБВГ = "cyrillic"


# ═══════════════════════════════════════════════════════════════════════
# EDGE CASE: EMPTY BODIES & STUBS
# ═══════════════════════════════════════════════════════════════════════

# All valid empty body forms
def stub1(): pass
def stub2(): ...
def stub3(): return
def stub4():
    """Docstring only."""
def stub5():
    """Docstring."""
    ...
def stub6():
    """Docstring."""
    pass

class StubClass1: pass
class StubClass2: ...
class StubClass3:
    """Docstring."""
class StubClass4:
    """Docstring."""
    ...

# Interface-like pattern (typing stubs)
class Interface:
    def method1(self) -> None: ...
    def method2(self, x: int) -> str: ...
    def method3(self) -> list[int]: ...

# Overload pattern
@overload
def ov(x: int) -> int: ...
@overload
def ov(x: str) -> str: ...
def ov(x):
    return x


# ═══════════════════════════════════════════════════════════════════════
# EVERYTHING AT MODULE LEVEL
# ═══════════════════════════════════════════════════════════════════════

# Module-level match
match sys.platform:
    case "linux":
        _PLATFORM = "Linux"
    case "darwin":
        _PLATFORM = "macOS"
    case "win32":
        _PLATFORM = "Windows"
    case _:
        _PLATFORM = "Unknown"

# Module-level try
try:
    _RESULT = 1 / 1
except ZeroDivisionError:
    _RESULT = 0
else:
    _RESULT *= 2
finally:
    pass

# Module-level for
_SQUARES = {}
for _i in range(10):
    _SQUARES[_i] = _i ** 2

# Module-level while
_counter = 3
while _counter > 0:
    _counter -= 1

# Module-level with
with open(os.devnull) as _devnull:
    pass

# Module-level async def
async def module_level_async():
    return await asyncio.sleep(0, result=42)
