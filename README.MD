# BeautifulString Library

Welcome to the `BeautifulString` library, a high-performance Python extension written in C designed for advanced string manipulation and textual analysis. This library provides powerful, fast, and intuitive tools to handle everything from simple string collections to complex data processing and analysis tasks.

The library currently features two main classes:
* **`BString`**: A versatile, mutable container for holding sequences of strings, with rich support for data export, file I/O, and list-like operations.
* **`BeautifulAnalyzer`**: A simple yet powerful tool for calculating key statistics from a body of text.

## Features

* **High-Performance**: Core logic is written in C for maximum speed, especially for large datasets and file I/O.
* **Full Mutability**: `BString` objects can be modified in-place with methods like `.append()`, `.insert()`, `.pop()`, and full slicing support.
* **Rich Data Export**: Convert `BString` objects on-the-fly to Python `list`, `tuple`, `dict`, and to `JSON` or highly configurable `CSV` string formats.
* **Advanced Data I/O**: Robust, multi-line `CSV` reading and writing capabilities, including header support.
* **Powerful Transformations**: Use `.map()` and `.filter()` to apply functions and methods across all strings in a collection.
* **Textual Analysis**: Instantly get character, word, and sentence counts for any block of text with `BeautifulAnalyzer`.

## Building from Source

This project uses CMake for building.

1.  **Clone the repository:**
    ```bash
    git clone <your-repo-url>
    cd <your-repo-directory>
    ```

2.  **Configure with CMake:**
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  **Build the project:**
    ```bash
    cmake --build .
    ```

After a successful build, the Python extension module (e.g., `BeautifulString.pyd` or `BeautifulString.so`) will be located in the `build` directory. You can then import it in your Python scripts.

## Usage & API

# BString Class

`BString` is a high-performance, mutable container for holding an ordered sequence of strings. ⛓️ It's written as a Python C extension to provide maximum speed for data-heavy tasks, while offering a rich, Pythonic API.

It's designed for efficient data manipulation, transformation, and serialization to and from various formats. `BString` is a core component of the `BeautifulString` library.

***
## Features

* **High-Performance**: Core logic is written in C for speed.
* **Full Mutability**: Supports `.append()`, `.insert()`, `.pop()`, `.remove()`, and item assignment.
* **Complete Slicing**: Full `list`-like slice support for getting, setting, and deleting.
* **Rich Data Export**: Convert instances on-the-fly to `list`, `tuple`, `dict`, `JSON`, and highly configurable `CSV` formats.
* **Advanced File I/O**: Robust methods for reading/writing text files and multi-line `CSV` files with header support.
* **Powerful Transformations**: Includes `.map()`, `.filter()`, `.extend()`, and a `.transform_chars()` method for bulk character removal/retention.
* **Linked-List Navigation**: Unique `.head`, `.tail`, `.next`, and `.prev` properties for cursor-style iteration.

***
## API and Usage

### Initialization & Basic Operations

`BString` is initialized with one or more string arguments. It behaves like a standard Python sequence.

```python
from BeautifulString import BString

# Initialization
b = BString('apple', 'banana', 'cherry')
print(b)
# Output: ['apple', 'banana', 'cherry']

# Get length
print(f"Length: {len(b)}") # Output: 3

# Iteration
for fruit in b:
    print(fruit.upper())
    
# BeautifulAnalyzer

`# BeautifulAnalyzer

`BeautifulAnalyzer` is a high-performance Python class, written in C, for calculating a suite of advanced statistics and metrics from a body of text. It's designed to be fast, efficient, and easy to use, accepting various text formats as input.

It is part of the `BeautifulString` library.

***
## Features

* **Basic Counts**: Instantly get character, word, and sentence counts.
* **Lexical Diversity**: Calculate the number of unique words and the average sentence length.
* **Readability Score**: Determine the complexity of the text using the Flesch-Kincaid Grade Level formula.
* **Frequency Analysis**: Generate sorted word frequency distributions.
* **N-gram Generation**: Create lists of bigrams, trigrams, and other n-grams.
* **Flexible Input**: Initializes from a plain string, a list/tuple of strings, or a `BString` object.
* **High Performance**: C-level implementation with result caching for fast, repeated access to statistics.

***
## API and Usage

### Initialization

The analyzer is initialized with a `text` keyword argument. The input is joined into a single text block for analysis.

```python
from BeautifulString import BString, BeautifulAnalyzer

# Initialize with a plain string
analyzer_str = BeautifulAnalyzer(text="This is a sentence. This is another.")

# Initialize with a list of strings
analyzer_list = BeautifulAnalyzer(text=["First part.", "Second part."])

# Initialize with a BString object
b = BString("First BString part.", "Second BString part.")
analyzer_bstring = BeautifulAnalyzer(text=b)

## New String Utility Functions

The following C-accelerated Python functions extend BeautifulString with powerful parsing, scanning, validating, and formatting capabilities:

### `strfetch(input_str, input_slices)`
Extracts substrings using Python-style slice syntax in string form.
```python
bs.strfetch("abcdefg", "[1:5]")  # ['bcde']
bs.strfetch("abcdefg", "[::2],[1:4]")  # ['aceg', 'bcd']
```

### `strscan(input_str, format_str, return_type='list')`
Scans a string into tokens based on C-style format specifiers. Supports optional named fields and structured return types.
```python
bs.strscan("Age: 42", "Age: %d")  # [42]
bs.strscan("Name: Alice Age: 30", "Name: %s(name) Age: %d(age)", return_type="dict")
```

### `strmatch(input_str, pattern, return_type='list')`
Matches a string against a fixed format. Works like sscanf with named fields and can return dicts, tuples, or tuple-lists.
```python
bs.strmatch("User: Bob Age: 28", "User: %s(name) Age: %d(age)", return_type="dict")
```

### `strsearch(input_str, pattern, return_type='list', max_matches=-1)`
Searches and extracts multiple pattern matches from a longer string.
```python
bs.strsearch("Name: Alice Score: 99 Name: Bob Score: 88", "Name: %s(name) Score: %d(score)", return_type="dict")
```

### `strvalidate(input_str, format_str)`
Validates a string format using specifiers, optionally including constraints:
```python
bs.strvalidate("Age: 25", "Age: %d(min=18,max=99)")  # True
bs.strvalidate("ID: abc123", "ID: %w(len=6)")        # True for 6-char alphanumeric
```

### `strparse(input_str, format_str, return_type='dict')`
Parses a structured string using format specifiers, returns a structured result.
```python
bs.strparse("Temp 36.6", "Temp %f", return_type="tuple")  # (36.6,)
bs.strparse("X 100 Y 200", "X %d(x) Y %d(y)", return_type="tuple_list")
```
# strlearn Function

The `strlearn` function is a utility within the `BeautifulString` library that automatically analyzes a list of example strings to infer the underlying data types of their columns. It's a powerful tool for understanding the structure of semi-structured text data, like log files or simple reports, without needing to define a parser manually.

***
## Features

* Learns column data types (**int**, **float**, **string**) from examples.
* Tokenizes strings based on whitespace and quotes.
* Handles ragged data (rows with different numbers of columns) by defaulting to the safest type (`string`).
* Offers multiple output formats for flexibility.

***
## API and Usage

### Signature
```python
BeautifulString.strlearn(list_of_strings, format='list')
---

## Return Types

All pattern-based functions support:
- `'list'` (default): ordered values
- `'tuple'`: like list but as a tuple
- `'dict'`: uses named fields
- `'tuple_list'`: list of `(key, value)` pairs

---

## Format Specifiers

| Specifier | Meaning               |
|-----------|------------------------|
| `%s`      | String (supports quotes & escapes) |
| `%d`      | Integer                |
| `%f`      | Float (32-bit)         |
| `%lf`     | Double (64-bit float)  |
| `%u`      | Unsigned integer       |
| `%x`      | Hexadecimal            |
| `%w`      | Alphanumeric word      |

Constraints like `min=`, `max=`, `len=`, `in=` can be added in parentheses for validation.
