from BeautifulString import BString

b = BString('  Apple  ', '   Pear ', ' GRAPES  ', 'berry')
print(f"Original: {b}")

# --- Test .map() ---
print("\n--- Testing .map() ---")
# Map with a method that takes no arguments
stripped = b.map('strip')
print(f"b.map('strip'): {stripped}")

# Map with a method that takes arguments
replaced = stripped.map('replace', 'p', 'P')
print(f"stripped.map('replace', 'p', 'P'): {replaced}")


# --- Test .filter() ---
print("\n--- Testing .filter() ---")
# Filter with a method that takes no arguments
uppers = b.filter('isupper')
print(f"b.filter('isupper'): {uppers}")

# Filter with a method that takes an argument
p_words = stripped.filter('startswith', 'P')
print(f"stripped.filter('startswith', 'P'): {p_words}")