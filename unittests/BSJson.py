from BeautifulString import BString

# --- Test Case 1: Default JSON array output ---
print("--- Test Case 1: Default JSON Array ---")
# Include a string with a quote to test escaping
b1 = BString('apple', 'banana', 'cherry with a "quote"')
print(f"Original: {b1}")
json_array = b1(container='json')
print(f"JSON Array: {json_array}")
print(f"Type of result: {type(json_array)}")


# --- Test Case 2: JSON object output with keys ---
print("\n--- Test Case 2: JSON Object with Keys ---")
b2 = BString('Rovaniemi', 'Finland')
keys = ['city', 'country']
print(f"Original: {b2}")
print(f"Keys: {keys}")
json_object = b2(container='json', keys=keys)
print(f"JSON Object: {json_object}")

# --- Test Case 3: Error handling ---
print("\n--- Test Case 3: Error Handling ---")
try:
    b2(container='json', keys=['city']) # Mismatched key length
except ValueError as e:
    print(f"Correctly caught error: {e}")