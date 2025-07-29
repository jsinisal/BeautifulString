from BeautifulString import BString

# --- Test BString.split() ---
log_line = "INFO;user-login;lehto;session-123"
b = BString.split(log_line, delimiter=";")
print(f"--- Testing .split() ---")
print(f"Original string: '{log_line}'")
print(f"Result of split: {b}")
assert list(b) == ['INFO', 'user-login', 'lehto', 'session-123']

# --- Test b.join() ---
joined_string = b.join(" | ")
print(f"\n--- Testing .join() ---")
print(f"Original BString: {b}")
print(f"Result of join: '{joined_string}'")
assert joined_string == 'INFO | user-login | lehto | session-123'

print("\nAll assertions passed.")