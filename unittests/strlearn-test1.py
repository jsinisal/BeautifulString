import BeautifulString as bs

example_data = [
    "ID: 101 Name: Alice",
    "ID: 205 Name: Bob"
]

# Default "list" format
list_format = bs.strlearn(example_data)
print(f"List Format: {list_format}")

# Optional "c-style" format
c_style_format = bs.strlearn(example_data, format='c-style')
print(f"C-Style Format: {c_style_format}")