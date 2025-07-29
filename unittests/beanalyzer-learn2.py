from BeautifulString import BeautifulAnalyzer

# A list of similarly structured strings
log_lines = [
    "EVENT: LOGIN USER: bob SESSION: 101",
    "EVENT: LOGOUT USER: alice SESSION: 345",
    "EVENT: PURCHASE USER: charlie SESSION: 812"
]

# Call the static method directly on the class
inferred_types = BeautifulAnalyzer.learn_format(log_lines)

print(f"Input: {log_lines[0]}")
print(f"Inferred column types: {inferred_types}")