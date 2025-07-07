import os
from BeautifulString import BString

# Define a temporary file path and its content
FILE_PATH = "people.csv"
CSV_CONTENT = """Name,City,Country
Anna,Helsinki,Finland
Bjorn,Stockholm,Sweden
Chloe,Copenhagen,Denmark
"""

# Create the test file
with open(FILE_PATH, "w", newline="") as f:
    f.write(CSV_CONTENT)

print(f"Created test file '{FILE_PATH}'\n")

# --- Test 1: Load with header=True (default) ---
print("--- Testing BString.from_csv(header=True) ---")
try:
    header, data_rows = BString.from_csv(FILE_PATH)

    print(f"Header: {header}")
    print("Data Rows:")
    for i, row in enumerate(data_rows):
        print(f"  Row {i}: {row}")

    # Verification
    assert list(header) == ["Name", "City", "Country"]
    assert len(data_rows) == 3
    assert list(data_rows[1]) == ["Bjorn", "Stockholm", "Sweden"]
    print("SUCCESS: Data loaded and parsed correctly.")

except Exception as e:
    print(f"An error occurred: {e}")

# --- Test 2: Load with header=False ---
print("\n--- Testing BString.from_csv(header=False) ---")
try:
    all_rows = BString.from_csv(FILE_PATH, header=False)

    print("All Rows (header treated as data):")
    for i, row in enumerate(all_rows):
        print(f"  Row {i}: {row}")

    # Verification
    assert len(all_rows) == 4  # Header + 3 data rows
    assert list(all_rows[0]) == ["Name", "City", "Country"]
    print("SUCCESS: All lines loaded as data correctly.")

except Exception as e:
    print(f"An error occurred: {e}")


# --- Clean up ---
finally:
    if os.path.exists(FILE_PATH):
        os.remove(FILE_PATH)
        print(f"\nCleaned up '{FILE_PATH}'.")