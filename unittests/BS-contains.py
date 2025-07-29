from BeautifulString import BString

b = BString("Helsinki, Finland", "Oulu", "Rovaniemi, Lapland")

# --- Test Case 1: Case-sensitive search (success) ---
# Does any string contain "land"?
print(f"Contains 'land'? -> {b.contains('land')}")
assert b.contains('land') is True

# --- Test Case 2: Case-sensitive search (failure) ---
# Does any string contain "lapland" (lowercase)?
print(f"Contains 'lapland'? -> {b.contains('lapland')}")
assert b.contains('lapland') is False

# --- Test Case 3: Case-insensitive search (success) ---
# Does any string contain "lapland" (case-insensitive)?
print(f"Contains 'lapland' (case-insensitive)? -> {b.contains('lapland', case_sensitive=False)}")
assert b.contains('lapland', case_sensitive=False) is True

print("\nAll tests passed.")