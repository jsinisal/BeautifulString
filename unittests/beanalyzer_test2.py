from BeautifulString import BeautifulAnalyzer

text = "The quick brown fox jumps over the lazy dog. The quick brown fox is fast."

analyzer = BeautifulAnalyzer(text=text)

print(f"--- Analysis of: '{text}' ---")
print(f"Word Count: {analyzer.word_count}")
print(f"Unique Word Count: {analyzer.unique_word_count}")
print(f"Sentence Count: {analyzer.sentence_count}")
print(f"Average Sentence Length: {analyzer.average_sentence_length:.2f}")

print("\n--- Verification ---")
# 14 total words / 2 sentences = 7.0
assert analyzer.average_sentence_length == 7.5
# a, brown, dog, fast, fox, is, jumps, lazy, over, quick, the
assert analyzer.unique_word_count == 10
print("All assertions passed.")