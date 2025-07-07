from BeautifulString import BString

# --- Setup ---
b = BString('one', 'two', 'three')
print(f"Initial:           {b}, size: {len(b)}")

# --- Test append ---
b.append('four')
print(f"After append('four'): {b}, size: {len(b)}")

# --- Test insert ---
b.insert(1, 'ONE_AND_A_HALF')
print(f"After insert(1, ...): {b}, size: {len(b)}")

# --- Test __setitem__ ---
b[0] = 'ZERO'
print(f"After b[0] = 'ZERO':  {b}, size: {len(b)}")

# --- Test pop ---
popped = b.pop() # Pop last
print(f"After pop():         {b}, popped '{popped}', size: {len(b)}")
popped = b.pop(1) # Pop by index
print(f"After pop(1):        {b}, popped '{popped}', size: {len(b)}")

# --- Test remove ---
b.remove('three')
print(f"After remove('three'):{b}, size: {len(b)}")

# --- Test __delitem__ ---
del b[0]
print(f"After del b[0]:      {b}, size: {len(b)}")

# --- Test error handling ---
try:
    b.remove('not_found')
except ValueError as e:
    print(f"Correctly caught error: {e}")