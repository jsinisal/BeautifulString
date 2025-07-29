from BeautifulString import BString

# --- Test 1: Remove vowels (default behavior, returns new BString) ---
print("--- Test: Remove Vowels ---")
b1 = BString("Hello World", "Welcome to Rovaniemi")
vowels = "aeiou" # Case-insensitive by default in this context
no_vowels = b1.transform_chars(vowels)

print(f"Original: {b1}")
print(f"No Vowels: {no_vowels}")
assert list(no_vowels) == ['Hll Wrld', 'Wlcm t Rvnm']


# --- Test 2: Keep only numbers (mode='keep') ---
print("\n--- Test: Keep Only Numbers ---")
b2 = BString("Order: #123-A", "Product ID: 456-B")
digits = "0123456789"
numbers_only = b2.transform_chars(digits, mode='keep')

print(f"Original: {b2}")
print(f"Numbers Only: {numbers_only}")
assert list(numbers_only) == ['123', '456']


# --- Test 3: Modify in-place ---
print("\n--- Test: In-place Modification ---")
b3 = BString("  remove whitespace  ", "  and tabs\t")
whitespace = " \t"
return_val = b3.transform_chars(whitespace, inplace=True)

print(f"Modified object: {b3}")
print(f"Return value: {return_val}")
assert list(b3) == ['removewhitespace', 'andtabs']
assert return_val is None

print("\nAll assertions passed.")