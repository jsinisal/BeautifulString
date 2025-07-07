import os
import filecmp
from BeautifulString import BString

# Define temporary file paths and initial content
FILE_PATH_ORIGINAL = "original.csv"
FILE_PATH_SAVED = "saved_roundtrip.csv"

CSV_CONTENT = """Name,City,Country
Anna,"Helsinki, Finland",Finland
Björn,Stockholm,Sweden
"""

# Create the original test file
with open(FILE_PATH_ORIGINAL, "w", newline="", encoding="utf-8") as f:
    f.write(CSV_CONTENT)

print(f"--- Testing CSV Round-Trip ---")
header_obj, data_rows = (None, None)

try:
    # 1. Load the original CSV into BString objects
    print(f"1. Loading from '{FILE_PATH_ORIGINAL}'...")
    header_obj, data_rows = BString.from_csv(FILE_PATH_ORIGINAL)
    print(f"   Header loaded: {header_obj}")
    print(f"   Data rows loaded: {len(data_rows)}")
    assert list(header_obj) == ["Name", "City", "Country"]
    assert len(data_rows) == 2

    # 2. Save the BString objects back to a new CSV file
    print(f"\n2. Saving data to '{FILE_PATH_SAVED}'...")
    BString.to_csv(FILE_PATH_SAVED, data=data_rows, header=header_obj)
    print("   Save complete.")

    # 3. Verify that the new file is identical to the original
    print("\n3. Verifying file integrity...")
    if filecmp.cmp(FILE_PATH_ORIGINAL, FILE_PATH_SAVED, shallow=False):
        print("SUCCESS: Saved file is identical to the original.")
    else:
        print("FAILURE: Saved file differs from the original.")

except Exception as e:
    print(f"An error occurred: {e}")

finally:
    # Clean up the test files
    for path in [FILE_PATH_ORIGINAL, FILE_PATH_SAVED]:
        if os.path.exists(path):
            os.remove(path)
            print(f"Cleaned up '{path}'.")