from BeautifulString import BString

fruits = BString("apple", "banana", "cherry", "kiwi")

# --- Access static start and end points ---
print(f"Head is always: {fruits.head}")
print(f"Tail is always: {fruits.tail}")

# The cursor is already at the head after creation
print(f"Immediately after creation, .current is: '{fruits.current}'")

# Now, let's move the cursor to the end
fruits.move_to_tail()
print(f"After moving to tail, .current is: '{fruits.current}'")

# Use .move_to_head() to reset the cursor
fruits.move_to_head()
print(f"After move_to_head(), .current is: '{fruits.current}'")

# Loop while there is a next item to move to
while fruits.move_next():
    print(f"Current item: {fruits.current}")

print(f"\nAfter loop, current item is: {fruits.current}")