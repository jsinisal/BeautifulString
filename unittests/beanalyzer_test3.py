from BeautifulString import BeautifulAnalyzer

# A sentence with a known score
# See: https://en.wikipedia.org/wiki/Flesch%E2%80%93Kincaid_readability_tests
text = "The Australian platypus is seemingly a hybrid of a mammal and a reptilian creature."
# This text has: 14 words, 1 sentence, 26 syllables
# FK Score = 0.39 * (14/1) + 11.8 * (26/14) - 15.59 = 5.46 + 21.91 - 15.59 = 11.78
# This corresponds to roughly a 12th-grade reading level.

analyzer = BeautifulAnalyzer(text=text)

score = analyzer.readability_score()
print(f"Readability Score (Flesch-Kincaid Grade Level): {score:.2f}")

# Verification
assert 11.0 < score < 12.5 # Check if it's in the expected range
print("Assertion passed.")

text = "the quick brown fox jumps over the lazy dog and the other quick fox"

analyzer = BeautifulAnalyzer(text=text)

print(f"--- Analysis of: '{text}' ---")

# Test getting all word frequencies
all_freqs = analyzer.word_frequency()
print("\nAll Word Frequencies:")
print(all_freqs)
assert all_freqs[0] == ('the', 3)
assert all_freqs[1] == ('quick', 2)
assert all_freqs[2] == ('fox', 2)

# Test getting the top 3 most frequent words
top_3 = analyzer.word_frequency(top_n=3)
print("\nTop 3 Words:")
print(top_3)
assert len(top_3) == 3
assert top_3[0] == ('the', 3)


print("\nAll assertions passed.")

text = "the quick brown fox jumps high"
analyzer = BeautifulAnalyzer(text=text)

print(f"--- Analysis of: '{text}' ---")

# --- Test bigrams (n=2, default) ---
bigrams = analyzer.generate_ngrams()
print("\nBigrams (n=2):")
for gram in bigrams:
    print(gram)
assert len(bigrams) == 5
assert list(bigrams[0]) == ['the', 'quick']
assert list(bigrams[4]) == ['jumps', 'high']

# --- Test trigrams (n=3) ---
trigrams = analyzer.generate_ngrams(n=3)
print("\nTrigrams (n=3):")
for gram in trigrams:
    print(gram)
assert len(trigrams) == 4
assert list(trigrams[0]) == ['the', 'quick', 'brown']
assert list(trigrams[3]) == ['fox', 'jumps', 'high']

print("\nAll assertions passed.")