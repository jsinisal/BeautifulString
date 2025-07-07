import unittest
import BeautifulString as bs

class TestStrValidate(unittest.TestCase):
    def test_integer_validation(self):
        self.assertTrue(bs.strvalidate("Age: 42", "Age: %d"))
        self.assertFalse(bs.strvalidate("Age: forty-two", "Age: %d"))

    def test_string_validation(self):
        self.assertTrue(bs.strvalidate("Name: Alice", "Name: %s"))
        self.assertFalse(bs.strvalidate("Name:", "Name: %s"))

    def test_word_validation(self):
        self.assertTrue(bs.strvalidate("Code: abc123", "Code: %w"))
        self.assertFalse(bs.strvalidate("Code: abc 123", "Code: %w"))

    def test_combined_validation(self):
        self.assertTrue(bs.strvalidate("User: Bob Age: 28", "User: %s Age: %d"))
        self.assertFalse(bs.strvalidate("User: 123 Age: Bob", "User: %s Age: %d"))

    def test_extra_characters(self):
        self.assertFalse(bs.strvalidate("Age: 42!", "Age: %d"))

    def test_incomplete_input(self):
        self.assertFalse(bs.strvalidate("Age:", "Age: %d"))

    def test_strict_matching(self):
        self.assertFalse(bs.strvalidate("  Age: 42", "Age: %d"))
        self.assertFalse(bs.strvalidate("Age: 42 ", "Age: %d"))

if __name__ == '__main__':
    unittest.main()

