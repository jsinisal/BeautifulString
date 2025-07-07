import os
import random
import string
import time
from BeautifulString import BString

# --- Configuration ---
NUM_LINES = 100_000
LINE_LENGTH = 40
FILE_PATH = "large_test_file.txt"


def generate_large_file():
    """
    Generates a large text file with random content for testing.
    Returns the first and last lines for later verification.
    """
    print(f"Generating '{FILE_PATH}' with {NUM_LINES:,} lines...")

    # Define the character set for random strings
    chars = string.ascii_letters + string.digits

    first_line = ""
    last_line = ""

    start_time = time.perf_counter()
    with open(FILE_PATH, 'w') as f:
        for i in range(NUM_LINES):
            # Generate a random line of the specified length
            line = ''.join(random.choice(chars) for _ in range(LINE_LENGTH))

            if i == 0:
                first_line = line
            if i == NUM_LINES - 1:
                last_line = line

            f.write(line + '\n')

    end_time = time.perf_counter()
    print(f"File generation complete in {end_time - start_time:.4f} seconds.")
    return first_line, last_line


def run_bstring_benchmark(first_line_expected, last_line_expected):
    """
    Loads the generated file using BString.from_file, times the operation,
    and verifies the contents.
    """
    if not os.path.exists(FILE_PATH):
        print(f"Error: Test file '{FILE_PATH}' does not exist.")
        return

    print("\n--- Starting BString File I/O Benchmark ---")

    # --- Time the BString.from_file() operation ---
    print("Loading file with BString.from_file()...")
    start_load_time = time.perf_counter()

    b_loaded = BString.from_file(FILE_PATH)

    end_load_time = time.perf_counter()
    duration = end_load_time - start_load_time
    print(f"SUCCESS: Loaded {len(b_loaded):,} lines in {duration:.4f} seconds.")

    # --- Verify the loaded data ---
    print("\nVerifying data integrity...")

    correct = True
    # 1. Check total number of lines
    if len(b_loaded) != NUM_LINES:
        print(f"  [FAIL] Length mismatch! Expected {NUM_LINES}, got {len(b_loaded)}")
        correct = False
    else:
        print(f"  [OK] Length is correct ({len(b_loaded):,}).")

    # 2. Spot-check the first line
    if b_loaded[0] != first_line_expected:
        print(f"  [FAIL] First line mismatch!")
        print(f"    Expected: {first_line_expected}")
        print(f"    Got:      {b_loaded[0]}")
        correct = False
    else:
        print("  [OK] First line content is correct.")

    # 3. Spot-check the last line
    if b_loaded[-1] != last_line_expected:
        print(f"  [FAIL] Last line mismatch!")
        print(f"    Expected: {last_line_expected}")
        print(f"    Got:      {b_loaded[-1]}")
        correct = False
    else:
        print("  [OK] Last line content is correct.")

    if correct:
        print("\nVerification successful! The file was loaded correctly.")


if __name__ == "__main__":
    # Ensure you have compiled and installed your BeautifulString module
    try:
        # Step 1: Generate the test data
        first_line, last_line = generate_large_file()

        # Step 2: Run the benchmark and verification
        run_bstring_benchmark(first_line, last_line)

    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")

    finally:
        # Step 3: Clean up the large test file
        if os.path.exists(FILE_PATH):
            os.remove(FILE_PATH)
            print(f"\nCleaned up test file '{FILE_PATH}'.")