"""
Main entry point for the Camelot Python Desktop Application.

This is a basic Kivy application that provides cross-platform support
for Windows, Linux, and Android.
"""

from typing import Optional

from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.uix.textinput import TextInput
from kivy.uix.widget import Widget


class CamelotApp(App):
    """Main application class for Camelot."""

    def __init__(self, **kwargs: object) -> None:
        """Initialize the app."""
        super().__init__(**kwargs)
        self.result_label: Optional[Label] = None

    def build(self) -> Widget:
        """Build and return the main widget."""
        # Create main layout
        main_layout = BoxLayout(orientation="vertical", padding=10, spacing=10)

        # Add title
        title = Label(
            text="Camelot Python Desktop App",
            size_hint_y=None,
            height=50,
            font_size=24,
        )
        main_layout.add_widget(title)

        # Add description
        description = Label(
            text=(
                "Cross-platform desktop application\n"
                "Supports Windows, Linux, and Android"
            ),
            size_hint_y=None,
            height=80,
            halign="center",
        )
        main_layout.add_widget(description)

        # Add input field
        text_input = TextInput(
            hint_text="Enter some text here...",
            multiline=False,
            size_hint_y=None,
            height=40,
        )
        main_layout.add_widget(text_input)

        # Add button
        button = Button(text="Click Me!", size_hint_y=None, height=50)
        button.bind(on_press=lambda x: self.on_button_click(text_input.text))
        main_layout.add_widget(button)

        # Add result label
        self.result_label = Label(
            text="Welcome to Camelot!", size_hint_y=None, height=50
        )
        main_layout.add_widget(self.result_label)

        return main_layout

    def on_button_click(self, text: str) -> None:
        """Handle button click event."""
        if self.result_label is None:
            return

        if text.strip():
            self.result_label.text = f"You entered: {text}"
        else:
            self.result_label.text = "Please enter some text first!"


def main() -> None:
    """Main function to run the application."""
    CamelotApp().run()


if __name__ == "__main__":
    main()
