from BeautifulString import BString

b = BString('a', 'b', 'c', 'd', 'e', 'f')
print(f"Original: {b}")

# Test __getitem__ slicing
print(f"b[1:4] -> {b[1:4]}")
print(f"b[:3] -> {b[:3]}")
print(f"b[3:] -> {b[3:]}")
print(f"b[-1] -> {b[-1]}")
print(f"b[-3:-1] -> {b[-3:-1]}")
print(f"b[::2] -> {b[::2]}")

# Test __delitem__ slicing
del b[2:4]
print(f"After 'del b[2:4]': {b}")

# Test __setitem__ slicing
b[1:3] = BString('X', 'Y', 'Z')
print(f"After 'b[1:3] = BString(...)': {b}")

b[0] = 'A'
print(f"After 'b[0] = 'A'': {b}")


b1 = BString('a', 'b')
b2 = BString('c', 'd')

print(f"b1 = {b1}")
print(f"b2 = {b2}")

# Test concatenation
b_add = b1 + b2
print(f"b1 + b2 = {b_add}")
print(f"Length of b1 + b2 is {len(b_add)}")

# Test repetition
b_mul = b1 * 3
print(f"b1 * 3 = {b_mul}")
print(f"Length of b1 * 3 is {len(b_mul)}")

# Test repetition with 0
b_zero = b1 * 0
print(f"b1 * 0 = {b_zero}")
print(f"Length of b1 * 0 is {len(b_zero)}")