"""Exercise shipped configuration validation, including literal plus separators."""
import json
import unittest
from pathlib import Path

import jsonschema

ROOT = Path(__file__).resolve().parents[1]


class ConfigSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.schema = json.loads((ROOT / 'config.schema.json').read_text(encoding='utf-8'))
        jsonschema.Draft202012Validator.check_schema(cls.schema)
        cls.validator = jsonschema.Draft202012Validator(cls.schema)

    def test_legitimate_chords_are_accepted(self):
        # Catches double-escaped '+' regexes rejecting real ModMenu output.
        for token in ('NONE', 'LCTRL', 'BACKSPACE', 'K', 'CTRL+K', 'SHIFT+K',
                      'CTRL+SHIFT+K', 'LALT+K', 'CTRL+LALT+K', 'SHIFT+LALT+K',
                      'CTRL+SHIFT+LALT+K', 'CTRL+SHIFT+OEM_PLUS', 'CTRL+LALT',
                      'LALT+RALT', 'F24', 'NUMPAD9', 'VK_E9'):
            with self.subTest(token=token):
                self.assertTrue(self.validator.is_valid({'mode': 1, 'toggleKey': token}))

    def test_malformed_or_unobservable_bindings_are_rejected(self):
        for token in ('CTRL\\K', 'CTRL++K', 'SHIFT+CTRL+K', 'CTRL+NONE',
                      'CTRL+LCTRL', 'LALT+LALT', 'LALT+ALT', 'CTRL+SHIFT',
                      'ESCAPE', 'VK_1B', 'MOUSE1', 'F25', 'F01', '', 'K\n',
                      'CTRL+SHIFT+LALT+NONE', 'CTRL+' + 'K' * 49):
            with self.subTest(token=token):
                self.assertFalse(self.validator.is_valid({'toggleKey': token}))

    def test_existing_defaults_and_mode_validation(self):
        self.assertTrue(self.validator.is_valid({}))
        self.assertTrue(self.validator.is_valid({'mode': 2, 'toggleKey': 'NONE'}))
        for config in ({'mode': True}, {'mode': 3}, {'mode': -1},
                       {'toggleKey': 162}, {'enabled': True}):
            with self.subTest(config=config):
                self.assertFalse(self.validator.is_valid(config))


if __name__ == '__main__':
    unittest.main()
