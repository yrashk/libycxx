"""Regression tests for the negative compilation oracle (requires the suite's lit package)."""
import unittest

import lit.Test
from ycxxlit.ycxx_format import YcxxFormat


class ExpectedErrorsTests(unittest.TestCase):
    def check(self, source, diagnostics, compiler='gcc', command='test.cpp', features=()):
        fmt = YcxxFormat('ycxx-cxx', compiler, [], features=features)
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

    def test_exception_configuration_selects_its_diagnostic(self):
        source = ('// EXPECT-ERROR-GCC[exceptions]: uncaught exception\n'
                  '// EXPECT-ERROR-GCC[!exceptions]: non-constexpr error handler')
        self.assertEqual(self.check(source, 'error: uncaught exception', features=['exceptions']).code,
                         lit.Test.PASS)
        self.assertEqual(self.check(source, 'error: non-constexpr error handler').code, lit.Test.PASS)
        self.assertEqual(self.check(source, 'error: uncaught exception').code, lit.Test.FAIL)
        self.assertEqual(self.check(source, 'error: non-constexpr error handler',
                                    features=['exceptions']).code, lit.Test.FAIL)

    def test_conditional_diagnostic_still_requires_the_specific_reason(self):
        source = ('// EXPECT-ERROR[!exceptions]: non-constexpr error handler\n'
                  '// EXPECT-ERROR[!exceptions]: invalid presentation type')
        self.assertEqual(self.check(source, 'error: non-constexpr error handler\n'
                                    'note: invalid presentation type').code, lit.Test.PASS)
        self.assertEqual(self.check(source, 'error: non-constexpr error handler\n'
                                    'note: unmatched brace').code, lit.Test.FAIL)

    def test_feature_expression_and_compiler_both_apply(self):
        source = ('// EXPECT-ERROR-GCC[exceptions && !asan]: gcc error\n'
                  '// EXPECT-ERROR-CLANG[exceptions]: clang error')
        self.assertEqual(self.check(source, 'gcc error', features=['exceptions']).code, lit.Test.PASS)
        self.assertEqual(self.check(source, 'gcc error', features=['exceptions', 'asan']).code,
                         lit.Test.FAIL)
        self.assertEqual(self.check(source, 'gcc error', compiler='clang', features=['exceptions']).code,
                         lit.Test.FAIL)

    def test_invalid_or_empty_feature_expression_fails(self):
        for expr in ('', 'exceptions &&'):
            with self.subTest(expr=expr):
                result = self.check(f'// EXPECT-ERROR[{expr}]: error', 'error')
                self.assertEqual(result.code, lit.Test.FAIL)
                self.assertIn('EXPECT-ERROR[', result.output)

    def test_comma_is_conjunction_as_in_requires(self):
        source = '// EXPECT-ERROR[exceptions, !asan]: error'
        self.assertEqual(self.check(source, 'error', features=['exceptions']).code, lit.Test.PASS)
        self.assertEqual(self.check(source, 'error', features=['exceptions', 'asan']).code, lit.Test.FAIL)


if __name__ == '__main__':
    unittest.main()
