import unittest
from tira_measure import Environment

class Tests(unittest.TestCase):
    def test_py_measure_can_be_instantiated(self):
        environment = Environment()
        self.assertIsNotNone(environment)

    def test_py_measure_works_with_start_and_stop(self):
        environment = Environment()
        environment.start_measuring()
        actual = environment.stop_measuring()

        self.assertIsNotNone(actual)
        self.assertTrue('git' in actual)

    def test_py_measure_works_in_with_statement(self):
        environment = Environment()

        with environment.measure():
            pass

        self.assertEqual(1, len(environment.measurements))
        actual = environment.measurements[0]

        self.assertIsNotNone(actual)
        self.assertTrue('git' in actual)

    def test_py_measure_works_as_function_decorator(self):
        environment = Environment()

        @environment.measure
        def my_function(a, b):
            return a + b

        self.assertEqual(3, my_function(1,2))
        self.assertEqual(1, len(environment.measurements))
        actual = environment.measurements[0]

        self.assertIsNotNone(actual)
        self.assertTrue('git' in actual)
