from BeautifulString import BString

b = BString("apple", "banana", "apple", "cherry", "banana", "apple")
print(f"Original: {b}")

# Get the BString with unique values
unique_b = b.unique()
print(f"Unique:   {unique_b}")

# Verify the result
assert list(unique_b) == ["apple", "banana", "cherry"]
print("\nAssertion passed: Duplicates removed and order preserved.")