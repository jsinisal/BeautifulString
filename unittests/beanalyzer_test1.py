from BeautifulString import BString, BeautifulAnalyzer

# --- Setup Test Data ---
text_str = "This is the first sentence. Here is the second sentence! Is this the third?"
text_list = ["This is a list.", "It gets joined together."]
text_bstring = BString("This is a BString.", "It also gets joined.")

# --- Create Analyzers with different input types ---
analyzer_str = BeautifulAnalyzer(text_str)
analyzer_list = BeautifulAnalyzer(text_list)
analyzer_bstring = BeautifulAnalyzer(text_bstring)

# --- Test Properties ---
print("--- Analysis of a plain string ---")
print(f"Content: '{analyzer_str.text_blob}'") # Assuming you add text_blob as a property for testing
print(f"Word Count: {analyzer_str.word_count}")
assert analyzer_str.word_count == 14

print("\n--- Analysis of a list of strings ---")
print(f"Content: '{analyzer_list.text_blob}'")
print(f"Word Count: {analyzer_list.word_count}")
assert analyzer_list.word_count == 8

print("\n--- Analysis of a BString ---")
print(f"Content: '{analyzer_bstring.text_blob}'")
print(f"Word Count: {analyzer_bstring.word_count}")
assert analyzer_bstring.word_count == 8

print("\nSUCCESS: BeautifulAnalyzer correctly handles all input types.")