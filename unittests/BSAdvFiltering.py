from BeautifulString import BString


a = BString("koe stringi")
print(a)

b = BString('Apple', 'Pear', 'GRAPES', 'Blueberry', 'Raspberry')
print(f"Original: {b}")

# --- Test 1: Filtering by method name (as before) ---
print("\n--- Filtering by method name ---")
p_words = b.filter('startswith', 'P')
print(f"b.filter('startswith', 'P'): {p_words}")

# --- Test 2: Filtering with a named function ---
print("\n--- Filtering with a named function ---")

def has_more_than_5_chars(s):
    return len(s) > 5

long_words = b.filter(has_more_than_5_chars)
print(f"b.filter(has_more_than_5_chars): {long_words}")

# --- Test 3: Filtering with a lambda function ---
print("\n--- Filtering with a lambda function ---")
berry_words = b.filter(lambda s: s.endswith('berry'))
print(f"b.filter(lambda s: s.endswith('berry')): {berry_words}")

# --- Test 4: Error handling ---
print("\n--- Filtering with an invalid argument ---")
try:
    b.filter(123) # Pass an integer
except TypeError as e:
    print(f"Correctly caught error: {e}")