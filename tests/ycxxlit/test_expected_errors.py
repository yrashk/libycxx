"""Regression tests for the negative compilation oracle (requires the suite's lit package)."""
import unittest

import lit.Test
from ycxxlit.ycxx_format import YcxxFormat


class ExpectedErrorsTests(unittest.TestCase):
    def check(self, source, diagnostics, compiler='gcc', command='test.cpp'):
        fmt = YcxxFormat('ycxx-cxx', compiler, [])
        return fmt.check_expected_errors(source, f'$ {command}\n[compile: exit 1]\n{diagnostics}')

    def test_no_directive_cannot_accept_an_unrelated_failure(self):
        result = self.check('', 'error: unrelated declaration')
        self.assertEqual(result.code, lit.Test.FAIL)
        self.assertIn('no EXPECT-ERROR pattern applies to gcc', result.output)

    def test_other_compilers_directive_does_not_supply_an_oracle(self):
        result = self.check('// EXPECT-ERROR-GCC: deleted constructor',
                            'error: unrelated declaration', compiler='clang')
        self.assertEqual(result.code, lit.Test.FAIL)
        self.assertIn('no EXPECT-ERROR pattern applies to clang', result.output)

    def test_empty_pattern_is_not_a_diagnostic(self):
        self.assertEqual(self.check('// EXPECT-ERROR: ', 'error: unrelated').code, lit.Test.FAIL)

    def test_empty_pattern_cannot_use_the_next_source_line_as_an_oracle(self):
        source = '// EXPECT-ERROR:\nint unrelated;\n'
        result = self.check(source, 'error: invalid declaration\nint unrelated;')
        self.assertEqual(result.code, lit.Test.FAIL)
        self.assertIn('empty pattern', result.output)

    def test_empty_pattern_before_another_directive_is_still_empty(self):
        source = '// EXPECT-ERROR: \t\n// EXPECT-ERROR-GCC: deleted constructor\n'
        result = self.check(source, 'error: deleted constructor')
        self.assertEqual(result.code, lit.Test.FAIL)
        self.assertIn('empty pattern', result.output)

    def test_every_applicable_pattern_must_match(self):
        source = '// EXPECT-ERROR: deleted constructor\n// EXPECT-ERROR-GCC: dangling reference'
        self.assertEqual(self.check(source, 'error: deleted constructor').code, lit.Test.FAIL)
        self.assertEqual(self.check(source, 'error: deleted constructor\nnote: dangling reference').code,
                         lit.Test.PASS)

    def test_unrelated_failure_cannot_match_the_expected_rejection(self):
        self.assertEqual(self.check('// EXPECT-ERROR: deleted constructor',
                                    'error: unknown variable').code, lit.Test.FAIL)

    def test_command_text_is_not_diagnostic_evidence(self):
        self.assertEqual(self.check('// EXPECT-ERROR: deleted constructor',
                                    'error: unknown variable', command='deleted constructor.cpp').code,
                         lit.Test.FAIL)

    def test_invalid_regex_is_a_test_failure(self):
        result = self.check('// EXPECT-ERROR: [', 'error: deleted constructor')
        self.assertEqual(result.code, lit.Test.FAIL)
        self.assertIn('invalid regex', result.output)

    def test_compiler_specific_wording_and_shared_pattern(self):
        source = ('// EXPECT-ERROR: constructor\n// EXPECT-ERROR-GCC: use of deleted function\n'
                  '// EXPECT-ERROR-CLANG: call to deleted constructor')
        self.assertEqual(self.check(source, 'error: use of deleted function constructor').code, lit.Test.PASS)
        self.assertEqual(self.check(source, 'error: call to deleted constructor', compiler='clang').code,
                         lit.Test.PASS)


if __name__ == '__main__':
    unittest.main()
