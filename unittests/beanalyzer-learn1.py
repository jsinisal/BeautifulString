import unittest
import os
from io import StringIO
from BeautifulString import BString


class TestBString(unittest.TestCase):

    def test_initialization_and_repr(self):
        b = BString('hello', 'world')
        self.assertEqual(len(b), 2)
        self.assertEqual(repr(b), "['hello', 'world']")

    def test_mutability(self):
        b = BString('a', 'c')
        b.append('d')
        self.assertEqual(list(b), ['a', 'c', 'd'])
        b.insert(1, 'b')
        self.assertEqual(list(b), ['a', 'b', 'c', 'd'])
        popped = b.pop()
        self.assertEqual(popped, 'd')
        self.assertEqual(list(b), ['a', 'b', 'c'])
        b.remove('b')
        self.assertEqual(list(b), ['a', 'c'])
        b[0] = 'A'
        self.assertEqual(list(b), ['A', 'c'])

    def test_slicing_and_operators(self):
        b = BString('a', 'b', 'c', 'd', 'e')
        self.assertEqual(list(b[1:4]), ['b', 'c', 'd'])
        del b[2:4]
        self.assertEqual(list(b), ['a', 'b', 'e'])
        b[1:2] = BString('B', 'C')
        self.assertEqual(list(b), ['a', 'B', 'C', 'e'])

        b_sum = BString('x') + BString('y')
        self.assertEqual(list(b_sum), ['x', 'y'])
        b_mul = BString('z') * 3
        self.assertEqual(list(b_mul), ['z', 'z', 'z'])

    def test_navigation(self):
        b = BString('one', 'two', 'three')
        self.assertEqual(b.head, 'one')
        next_b = b.next
        self.assertEqual(next_b.head, 'two')
        prev_b = next_b.prev
        self.assertEqual(prev_b.head, 'one')
        self.assertEqual(b.tail, 'three')
        self.assertEqual(b.head, 'three')  # .tail moves the cursor

    def test_transformation(self):
        b = BString('  Apple  ', 'Pear')
        stripped = b.map('strip')
        self.assertEqual(list(stripped), ['Apple', 'Pear'])
        p_words = stripped.filter('startswith', 'P')
        self.assertEqual(list(p_words), ['Pear'])

    def test_data_export(self):
        b = BString('key1', 'value1')
        self.assertEqual(b(container='list'), ['key1', 'value1'])
        self.assertEqual(b(container='tuple'), ('key1', 'value1'))
        self.assertEqual(b(container='dict', keys=['k', 'v']), {'k': 'key1', 'v': 'value1'})
        self.assertEqual(b(container='json'), '["key1", "value1"]')
        self.assertEqual(b(container='csv', delimiter='|'), 'key1|value1')

    def test_file_io(self):
        FILE_PATH = "bstring_unittest.csv"
        header = BString("ID", "Name")
        data = [BString("1", "Alice"), BString("2", "Bob")]

        try:
            # Test to_csv and from_csv round-trip
            BString.to_csv(FILE_PATH, data=data, header=header)

            loaded_header, loaded_data = BString.from_csv(FILE_PATH, header=True)

            self.assertEqual(list(header), list(loaded_header))
            self.assertEqual(len(data), len(loaded_data))
            self.assertEqual(list(data[0]), list(loaded_data[0]))
        finally:
            if os.path.exists(FILE_PATH):
                os.remove(FILE_PATH)


if __name__ == '__main__':
    unittest.main()
    print()