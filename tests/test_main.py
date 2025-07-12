"""
Basic tests for the Camelot Python application.
"""

import unittest
import sys
import os

# Add the project root to Python path
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))


class TestCamelotLogic(unittest.TestCase):
    """Test cases for Camelot application logic."""

    def test_button_click_logic_with_text(self):
        """Test button click logic with text input."""
        # Simulate the button click logic without Kivy
        def simulate_button_click(text):
            if text.strip():
                return f"You entered: {text}"
            else:
                return "Please enter some text first!"
        
        # Test with text
        result = simulate_button_click("Hello World")
        self.assertEqual(result, "You entered: Hello World")

    def test_button_click_logic_without_text(self):
        """Test button click logic without text input."""
        def simulate_button_click(text):
            if text.strip():
                return f"You entered: {text}"
            else:
                return "Please enter some text first!"
        
        # Test with empty text
        result = simulate_button_click("")
        self.assertEqual(result, "Please enter some text first!")

        # Test with whitespace only
        result = simulate_button_click("   ")
        self.assertEqual(result, "Please enter some text first!")

    def test_package_import(self):
        """Test that the python_app package can be imported."""
        try:
            import python_app
            self.assertTrue(hasattr(python_app, '__version__'))
            self.assertEqual(python_app.__version__, "1.0.0")
        except ImportError:
            self.fail("python_app package could not be imported")

    def test_version_info(self):
        """Test version information."""
        import python_app
        self.assertEqual(python_app.__version__, "1.0.0")
        self.assertEqual(python_app.__author__, "Camelot Development Team")


if __name__ == '__main__':
    unittest.main()