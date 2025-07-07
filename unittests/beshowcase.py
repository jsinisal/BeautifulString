import io
from BeautifulString import BString, BeautifulAnalyzer

# Imagine this data comes from a file or an API call
csv_data = """Reviewer,Location,Comment
User1,Oulu,"A truly wonderful experience. Highly recommended!"
User2,Helsinki,"Good, but the service could be a bit faster."
User3,Rovaniemi,"Absolutely perfect! Best in Lapland."
"""

test = io.StringIO(csv_data)

# Use BString's powerful CSV loader
# We can use io.StringIO to treat the string as a file for this example
header, data_rows = BString.from_csv(io.StringIO(csv_data))

print(f"Loaded {len(data_rows)} reviews with the header: {header}")
print("-" * 30)

# Now, let's analyze the comment from the Rovaniemi user
rovaniemi_review_bstring = data_rows[2]
print(f"Analyzing review: {rovaniemi_review_bstring}")

# Pass the BString row directly to the analyzer
review_analyzer = BeautifulAnalyzer(rovaniemi_review_bstring)

# Get the stats!
print(f"  Character Count: {review_analyzer.char_count}")
print(f"  Word Count: {review_analyzer.word_count}")
print(f"  Sentence Count: {review_analyzer.sentence_count}")