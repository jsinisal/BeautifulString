import os
from BeautifulString import BString

# Define a temporary file path
FILE_PATH = "bstring_test.txt"

# --- Test 1: Save to file ---
print(f"--- Testing .to_file() ---")
b1 = BString('Hello from Rovaniemi', 'The sun is shining at 3 PM', 'This is a test.')
print(f"Original BString: {b1}")
try:
    b1.to_file(FILE_PATH)
    print(f"Successfully saved BString to '{FILE_PATH}'")
    # Verify file content
    with open(FILE_PATH, 'r') as f:
        print("--- File Content ---")
        print(f.read().strip())
        print("--------------------")
except Exception as e:
    print(f"An error occurred during save: {e}")

# --- Test 2: Load from file ---
print(f"\n--- Testing BString.from_file() ---")
try:
    b2 = BString.from_file(FILE_PATH)
    print(f"Successfully loaded BString from '{FILE_PATH}'")
    print(f"Loaded BString: {b2}")

    # --- Test 3: Verify the round trip ---
    print("\n--- Verifying correctness ---")
    # Using the __eq__ protocol which list implements
    if list(b1) == list(b2):
        print("SUCCESS: Original and loaded BStrings are identical.")
    else:
        print("FAILURE: Original and loaded BStrings differ.")

except Exception as e:
    print(f"An error occurred during load: {e}")

finally:
    # Clean up the test file
    if os.path.exists(FILE_PATH):
        os.remove(FILE_PATH)
        print(f"\nCleaned up '{FILE_PATH}'.")