from BeautifulString import BString

# Define quoting constants in Python to match our C-level ones
QUOTE_MINIMAL = 0
QUOTE_ALL = 1

# --- Test Case 1: Custom Delimiter ---
print("--- Test Case 1: Custom Delimiter ---")
b1 = BString('a', 'b', 'c')
print(f"Original: {b1}")
print(f"CSV with Semicolon: {b1(container='csv', delimiter=';')}")

# --- Test Case 2: Data requiring quoting (QUOTE_MINIMAL) ---
print("\n--- Test Case 2: QUOTE_MINIMAL (default) ---")
b2 = BString('field1', 'field,with,comma', 'field with "quotes"')
print(f"Original: {b2}")
print(f"Default CSV: {b2(container='csv')}")

# --- Test Case 3: QUOTE_ALL ---
print("\n--- Test Case 3: QUOTE_ALL ---")
b3 = BString('field1', 'field2', 'field3')
print(f"Original: {b3}")
print(f"QUOTE_ALL CSV: {b3(container='csv', quoting=QUOTE_ALL)}")

# --- Test Case 4: QUOTE_ALL with custom quotechar ---
print("\n--- Test Case 4: QUOTE_ALL with custom quotechar ---")
b4 = BString("value1", "value'2", "value3")
print(f"Original: {b4}")
print(f"QUOTE_ALL with single quotes: {b4(container='csv', quotechar='\'', quoting=QUOTE_ALL)}")