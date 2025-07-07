import unittest
import BeautifulString as bs

class TestStrParse(unittest.TestCase):
    def test_parse_dict(self):
        s = "Name Alice Age 30"
        result = bs.strparse(s, "Name %s(name) Age %d(age)", return_type="dict")
        self.assertEqual(result, {"name": "Alice", "age": 30})

    def test_parse_tuple(self):
        s = "Temperature 21.5"
        result = bs.strparse(s, "Temperature %f", return_type="tuple")
        self.assertEqual(result, (21.5,))

    def test_parse_unsigned_and_hex(self):
        s = "UID 12345 Color ff00ff"
        result = bs.strparse(s, "UID %u(uid) Color %x(color)", return_type="dict")
        self.assertEqual(result, {"uid": 12345, "color": 0xff00ff})

    def test_parse_tuple_list(self):
        s = "X 12 Y 34"
        result = bs.strparse(s, "X %d(x) Y %d(y)", return_type="tuple_list")
        self.assertEqual(result, [("x", 12), ("y", 34)])

    def test_parse_fails_on_mismatch(self):
        s = "Age: twenty"
        with self.assertRaises(ValueError):
            bs.strparse(s, "Age: %d(age)", return_type="dict")

    def test_invalid_return_type(self):
        s = "Test 123"
        with self.assertRaises(ValueError):
            bs.strparse(s, "Test %d(val)", return_type="unsupported")

if __name__ == "__main__":
    unittest.main()