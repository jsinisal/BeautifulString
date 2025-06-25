import unittest
import BeautifulString as bs

class TestStrMatch(unittest.TestCase):
    def test_dict_named_fields(self):
        s = "X: foo Y: 42"
        result = bs.strmatch(s, "X: %s(x) Y: %d(y)", return_container="dict")
        self.assertEqual(result, {'x': 'foo', 'y': 42})

    def test_list(self):
        s = "X: foo Y: 42"
        result = bs.strmatch(s, "X: %s Y: %d", return_container="list")
        self.assertEqual(result, ['foo', 42])

    def test_tuple(self):
        s = "X: foo Y: 42"
        result = bs.strmatch(s, "X: %s Y: %d", return_container="tuple")
        self.assertEqual(result, ('foo', 42))

    def test_tuple_list_named_fields(self):
        s = "X: foo Y: 42"
        result = bs.strmatch(s, "X: %s(x) Y: %d(y)", return_container="tuple_list")
        self.assertEqual(result, [('x', 'foo'), ('y', 42)])

    def test_quoted_string(self):
        s = 'Message: "Hello world!"'
        result = bs.strmatch(s, 'Message: %s(msg)', return_container="dict")
        self.assertEqual(result, {'msg': 'Hello world!'})

    def test_quoted_with_escape(self):
        s = 'Note: "Line1\\nLine2"'
        result = bs.strmatch(s, 'Note: %s(note)', return_container="dict")
        self.assertEqual(result, {'note': 'Line1\nLine2'})

    def test_hex_unsigned_float(self):
        s = "HEX: 1f UINT: 1234 FLOAT: 3.14"
        result = bs.strmatch(s, "HEX: %x(hex) UINT: %u(uint) FLOAT: %f(float)", return_container="dict")
        self.assertEqual(result, {'hex': 31, 'uint': 1234, 'float': 3.14})


class TestStrSearch(unittest.TestCase):
    def test_dict_named_fields(self):
        s = "User: Alice Score: 99 User: Bob Score: 88"
        result = bs.strsearch(s, "User: %s(name) Score: %d(score)", max_matches=2, return_container="dict")
        self.assertEqual(result, [{'name': 'Alice', 'score': 99}, {'name': 'Bob', 'score': 88}])

    def test_list(self):
        s = "User: Alice Score: 99 User: Bob Score: 88"
        result = bs.strsearch(s, "User: %s Score: %d", max_matches=2, return_container="list")
        self.assertEqual(result, [['Alice', 99], ['Bob', 88]])

    def test_tuple(self):
        s = "User: Alice Score: 99 User: Bob Score: 88"
        result = bs.strsearch(s, "User: %s Score: %d", max_matches=2, return_container="tuple")
        self.assertEqual(result, [('Alice', 99), ('Bob', 88)])

    def test_tuple_list_named_fields(self):
        s = "X: foo Y: 42 Z: bar"
        result = bs.strsearch(s, "X: %s(x) Y: %d(y) Z: %s(z)", return_container="tuple_list")
        self.assertEqual(result, [[('x', 'foo'), ('y', 42), ('z', 'bar')]])

class TestStrFetch(unittest.TestCase):
    def test_basic_slice(self):
        s = "abcdefg"
        result = bs.strfetch(s, "[1:4]")
        self.assertEqual(result, ['bcd'])

    def test_multiple_slices(self):
        s = "abcdefghij"
        result = bs.strfetch(s, "[0:3],[4:7]")
        self.assertEqual(result, ['abc', 'efg'])

    def test_negative_step(self):
        s = "abcdefg"
        result = bs.strfetch(s, "[::-1]")
        self.assertEqual(result, ['gfedcba'])

    def test_out_of_bounds(self):
        s = "short"
        result = bs.strfetch(s, "[0:100]")
        self.assertEqual(result, ['short'])

    def test_empty_result(self):
        s = "data"
        result = bs.strfetch(s, "[10:20]")
        self.assertEqual(result, [''])

    def test_whitespace_and_format(self):
        s = "abcdefgh"
        result = bs.strfetch(s, " [1:4] , [4:8] ")
        self.assertEqual(result, ['bcd', 'efgh'])

    def test_strfetch_simple_slices(self):
        s = "abcdefg"
        self.assertEqual(bs.strfetch(s, "[2:5]"), ["cde"])
        self.assertEqual(bs.strfetch(s, "[::-1]"), ["gfedcba"])
        self.assertEqual(bs.strfetch(s, "[:3],[4:]"), ["abc", "efg"])

    def test_strfetch_edge_cases(self):
        s = "0123456789"
        self.assertEqual(bs.strfetch(s, "[::2]"), ["02468"])
        self.assertEqual(bs.strfetch(s, "[1:-1:2]"), ["1357"])

    def test_strfetch_empty(self):
        self.assertEqual(bs.strfetch("", "[0:3]"), [""])
        self.assertEqual(bs.strfetch("abc", "[5:10]"), [""])


class TestStrscan(unittest.TestCase):

    def test_strscan_list(self):
        s = 'Alice 30 5.5'
        result = bs.strscan(s, "%s %d %f", return_type="list")
        self.assertEqual(result, ['Alice', 30, 5.5])

    def test_strscan_tuple(self):
        s = 'Alice 30 5.5'
        result = bs.strscan(s, "%s %d %f", return_type="tuple")
        self.assertEqual(result, ('Alice', 30, 5.5))

    def test_strscan_dict_named_fields(self):
        s = 'Name: Alice Age: 30 Height: 5.5'
        fmt = 'Name: %s(name) Age: %d(age) Height: %f(height)'
        result = bs.strscan(s, fmt, return_type="dict")
        self.assertEqual(result, {'name': 'Alice', 'age': 30, 'height': 5.5})

    def test_strscan_tuple_list_named_fields(self):
        s = 'Name: Alice Age: 30 Height: 5.5'
        fmt = 'Name: %s(name) Age: %d(age) Height: %f(height)'
        result = bs.strscan(s, fmt, return_type="tuple_list")
        self.assertEqual(result, [('name', 'Alice'), ('age', 30), ('height', 5.5)])

    def test_strscan_quoted_strings(self):
        s = 'Name: "Alice Smith" Age: 30'
        fmt = 'Name: %s(name) Age: %d(age)'
        result = bs.strscan(s, fmt, return_type="dict")
        self.assertEqual(result, {'name': 'Alice Smith', 'age': 30})

    def test_strscan_escape_sequences(self):
        s = 'Greeting: "Hello\nWorld" Age: 25'
        fmt = 'Greeting: %s(greet) Age: %d(age)'
        result = bs.strscan(s, fmt, return_type="dict")
        self.assertEqual(result, {'greet': 'Hello\nWorld'.encode('utf-8').decode('unicode_escape'), 'age': 25})

    def test_strscan_unsigned_and_hex(self):
        s = 'Unsigned: 4294967295 Hex: ff'
        fmt = 'Unsigned: %u(uid) Hex: %x(hexval)'
        result = bs.strscan(s, fmt, return_type="dict")
        self.assertEqual(result, {'uid': 4294967295, 'hexval': 255})

    def test_strscan_invalid_token(self):
        s = 'Name: Alice Age: thirty'
        fmt = 'Name: %s(name) Age: %d(age)'
        with self.assertRaises(ValueError):
            bs.strscan(s, fmt, return_type="dict")

    def test_strscan_literal_mismatch(self):
        s = 'Fullname: Alice Age: 30'
        fmt = 'Name: %s(name) Age: %d(age)'
        with self.assertRaises(ValueError):
            bs.strscan(s, fmt, return_type="dict")

    def test_strscan_unknown_specifier(self):
        s = 'X: foo'
        fmt = 'X: %q(val)'
        with self.assertRaises(ValueError):
            bs.strscan(s, fmt, return_type="dict")

if __name__ == '__main__':
    unittest.main()
