/*
 * ----------------------------------------------------------------------
 * |\ /| mxjson-test.c
 * | X | JSON Test Suite
 * |/ \|
 * ----------------------------------------------------------------------
 */

#include <stdio.h>

#include "mxjson.h"
#include "mxutil.h"

typedef struct {
   char   *test_name;
   char   *json;
   size_t  len;
} testcase_t;


/*
 * The testcases below are derived from JSONTestSuite by Nicolas Seriot
 * https://github.com/nst/JSONTestSuite
 *
 * commit 32b6c1ad8868c11da370925ee4963c201db5efaa
 * Author: Nicolas Seriot <nicolas@seriot.ch>
 * Date:   Tue Nov 7 00:01:38 2017 +0100
 */

/*
MIT License

Copyright (c) 2016 Nicolas Seriot

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

static const testcase_t testcases[] = {
    { "i_number_double_huge_neg_exp", "[123.456e-789]", 14 },
    { "i_number_huge_exp", "[0.4e00669999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999969999999006]", 137 },
    { "i_number_neg_int_huge_exp", "[-1e+9999]", 10 },
    { "i_number_pos_double_huge_exp", "[1.5e+9999]", 11 },
    { "i_number_real_neg_overflow", "[-123123e100000]", 16 },
    { "i_number_real_pos_overflow", "[123123e100000]", 15 },
    { "i_number_real_underflow", "[123e-10000000]", 15 },
    { "i_number_too_big_neg_int", "[-123123123123123123123123123123]", 33 },
    { "i_number_too_big_pos_int", "[100000000000000000000]", 23 },
    { "i_number_very_big_negative_int", "[-237462374673276894279832749832423479823246327846]", 51 },
    { "i_object_key_lone_2nd_surrogate", "{\"\\uDFAA\":0}", 12 },
    { "i_string_1st_surrogate_but_2nd_missing", "[\"\\uDADA\"]", 10 },
    { "i_string_1st_valid_surrogate_2nd_invalid", "[\"\\uD888\\u1234\"]", 16 },
    { "u_string_UTF-16LE_with_BOM", "ˇ˛[\0\"\0È\0\"\0]\0", 12 },
    { "i_string_UTF-8_invalid_sequence", "[\"Êó•—à˙\"]", 10 },
    { "i_string_UTF8_surrogate_U+D800", "[\"Ì†Ä\"]", 7 },
    { "i_string_incomplete_surrogate_and_escape_valid", "[\"\\uD800\\n\"]", 12 },
    { "i_string_incomplete_surrogate_pair", "[\"\\uDd1ea\"]", 11 },
    { "i_string_incomplete_surrogates_escape_valid", "[\"\\uD800\\uD800\\n\"]", 18 },
    { "i_string_invalid_lonely_surrogate", "[\"\\ud800\"]", 10 },
    { "i_string_invalid_surrogate", "[\"\\ud800abc\"]", 13 },
    { "i_string_invalid_utf-8", "[\"ˇ\"]", 5 },
    { "i_string_inverted_surrogates_U+1D11E", "[\"\\uDd1e\\uD834\"]", 16 },
    { "i_string_iso_latin_1", "[\"È\"]", 5 },
    { "i_string_lone_second_surrogate", "[\"\\uDFAA\"]", 10 },
    { "i_string_lone_utf8_continuation_byte", "[\"Å\"]", 5 },
    { "i_string_not_in_unicode_range", "[\"Ùøøø\"]", 8 },
    { "i_string_overlong_sequence_2_bytes", "[\"¿Ø\"]", 6 },
    { "i_string_overlong_sequence_6_bytes", "[\"¸Éøøøø\"]", 10 },
    { "i_string_overlong_sequence_6_bytes_null", "[\"¸ÄÄÄÄÄ\"]", 10 },
    { "i_string_truncated-utf-8", "[\"‡ˇ\"]", 6 },
    { "u_string_utf16BE_no_BOM", "\0[\0\"\0È\0\"\0]", 10 },
    { "u_string_utf16LE_no_BOM", "[\0\"\0È\0\"\0]\0", 10 },
    { "i_structure_UTF-8_BOM_empty_object", "Ôªø{}", 5 },
    { "n_array_1_true_without_comma", "[1 true]", 8 },
    { "n_array_a_invalid_utf8", "[aÂ]", 4 },
    { "n_array_colon_instead_of_comma", "[\"\": 1]", 7 },
    { "n_array_comma_after_close", "[\"\"],", 5 },
    { "n_array_comma_and_number", "[,1]", 4 },
    { "n_array_double_comma", "[1,,2]", 6 },
    { "n_array_double_extra_comma", "[\"x\",,]", 7 },
    { "n_array_extra_close", "[\"x\"]]", 6 },
    { "n_array_extra_comma", "[\"\",]", 5 },
    { "n_array_incomplete", "[\"x\"", 4 },
    { "n_array_incomplete_invalid_value", "[x", 2 },
    { "n_array_inner_array_no_comma", "[3[4]]", 6 },
    { "n_array_invalid_utf8", "[ˇ]", 3 },
    { "n_array_items_separated_by_semicolon", "[1:2]", 5 },
    { "n_array_just_comma", "[,]", 3 },
    { "n_array_just_minus", "[-]", 3 },
    { "n_array_missing_value", "[   , \"\"]", 9 },
    { "n_array_newlines_unclosed", "[\"a\",\n4\n,1,", 11 },
    { "n_array_number_and_comma", "[1,]", 4 },
    { "n_array_number_and_several_commas", "[1,,]", 5 },
    { "n_array_spaces_vertical_tab_formfeed", "[\"a\"\\f]", 8 },
    { "n_array_star_inside", "[*]", 3 },
    { "n_array_unclosed", "[\"\"", 3 },
    { "n_array_unclosed_trailing_comma", "[1,", 3 },
    { "n_array_unclosed_with_new_lines", "[1,\n1\n,1", 8 },
    { "n_array_unclosed_with_object_inside", "[{}", 3 },
    { "n_incomplete_false", "[fals]", 6 },
    { "n_incomplete_null", "[nul]", 5 },
    { "n_incomplete_true", "[tru]", 5 },
    { "n_multidigit_number_then_00", "123\0", 4 },
    { "n_number_++", "[++1234]", 8 },
    { "n_number_+1", "[+1]", 4 },
    { "n_number_+Inf", "[+Inf]", 6 },
    { "n_number_-01", "[-01]", 5 },
    { "n_number_-1.0.", "[-1.0.]", 7 },
    { "n_number_-2.", "[-2.]", 5 },
    { "n_number_-NaN", "[-NaN]", 6 },
    { "n_number_.-1", "[.-1]", 5 },
    { "n_number_.2e-3", "[.2e-3]", 7 },
    { "n_number_0.1.2", "[0.1.2]", 7 },
    { "n_number_0.3e+", "[0.3e+]", 7 },
    { "n_number_0.3e", "[0.3e]", 6 },
    { "n_number_0.e1", "[0.e1]", 6 },
    { "n_number_0_capital_E+", "[0E+]", 5 },
    { "n_number_0_capital_E", "[0E]", 4 },
    { "n_number_0e+", "[0e+]", 5 },
    { "n_number_0e", "[0e]", 4 },
    { "n_number_1.0e+", "[1.0e+]", 7 },
    { "n_number_1.0e-", "[1.0e-]", 7 },
    { "n_number_1.0e", "[1.0e]", 6 },
    { "n_number_1_000", "[1 000.0]", 9 },
    { "n_number_1eE2", "[1eE2]", 6 },
    { "n_number_2.e+3", "[2.e+3]", 7 },
    { "n_number_2.e-3", "[2.e-3]", 7 },
    { "n_number_2.e3", "[2.e3]", 6 },
    { "n_number_9.e+", "[9.e+]", 6 },
    { "n_number_Inf", "[Inf]", 5 },
    { "n_number_NaN", "[NaN]", 5 },
    { "n_number_U+FF11_fullwidth_digit_one", "[Ôºë]", 5 },
    { "n_number_expression", "[1+2]", 5 },
    { "n_number_hex_1_digit", "[0x1]", 5 },
    { "n_number_hex_2_digits", "[0x42]", 6 },
    { "n_number_infinity", "[Infinity]", 10 },
    { "n_number_invalid+-", "[0e+-1]", 7 },
    { "n_number_invalid-negative-real", "[-123.123foo]", 13 },
    { "n_number_invalid-utf-8-in-bigger-int", "[123Â]", 6 },
    { "n_number_invalid-utf-8-in-exponent", "[1e1Â]", 6 },
    { "n_number_invalid-utf-8-in-int", "[0Â]\n", 5 },
    { "n_number_minus_infinity", "[-Infinity]", 11 },
    { "n_number_minus_sign_with_trailing_garbage", "[-foo]", 6 },
    { "n_number_minus_space_1", "[- 1]", 5 },
    { "n_number_neg_int_starting_with_zero", "[-012]", 6 },
    { "n_number_neg_real_without_int_part", "[-.123]", 7 },
    { "n_number_neg_with_garbage_at_end", "[-1x]", 5 },
    { "n_number_real_garbage_after_e", "[1ea]", 5 },
    { "n_number_real_with_invalid_utf8_after_e", "[1eÂ]", 5 },
    { "n_number_real_without_fractional_part", "[1.]", 4 },
    { "n_number_starting_with_dot", "[.123]", 6 },
    { "n_number_with_alpha", "[1.2a-3]", 8 },
    { "n_number_with_alpha_char", "[1.8011670033376514H-308]", 25 },
    { "n_number_with_leading_zero", "[012]", 5 },
    { "n_object_bad_value", "[\"x\", truth]", 12 },
    { "n_object_bracket_key", "{[: \"x\"}\n", 9 },
    { "n_object_comma_instead_of_colon", "{\"x\", null}", 11 },
    { "n_object_double_colon", "{\"x\"::\"b\"}", 10 },
    { "n_object_emoji", "{üá®üá≠}", 10 },
    { "n_object_garbage_at_end", "{\"a\":\"a\" 123}", 13 },
    { "n_object_key_with_single_quotes", "{key: 'value'}", 14 },
    { "n_object_missing_colon", "{\"a\" b}", 7 },
    { "n_object_missing_key", "{:\"b\"}", 6 },
    { "n_object_missing_semicolon", "{\"a\" \"b\"}", 9 },
    { "n_object_missing_value", "{\"a\":", 5 },
    { "n_object_no-colon", "{\"a\"", 4 },
    { "n_object_non_string_key", "{1:1}", 5 },
    { "n_object_non_string_key_but_huge_number_instead", "{9999E9999:1}", 13 },
    { "n_object_pi_in_key_and_trailing_comma", "{\"π\":\"0\",}", 10 },
    { "n_object_repeated_null_null", "{null:null,null:null}", 21 },
    { "n_object_several_trailing_commas", "{\"id\":0,,,,,}", 13 },
    { "n_object_single_quote", "{'a':0}", 7 },
    { "n_object_trailing_comma", "{\"id\":0,}", 9 },
    { "n_object_trailing_comment", "{\"a\":\"b\"}/**/", 13 },
    { "n_object_trailing_comment_open", "{\"a\":\"b\"}/**//", 14 },
    { "n_object_trailing_comment_slash_open", "{\"a\":\"b\"}//", 11 },
    { "n_object_trailing_comment_slash_open_incomplete", "{\"a\":\"b\"}/", 10 },
    { "n_object_two_commas_in_a_row", "{\"a\":\"b\",,\"c\":\"d\"}", 18 },
    { "n_object_unquoted_key", "{a: \"b\"}", 8 },
    { "n_object_unterminated-value", "{\"a\":\"a", 7 },
    { "n_object_with_single_string", "{ \"foo\" : \"bar\", \"a\" }", 22 },
    { "n_object_with_trailing_garbage", "{\"a\":\"b\"}#", 10 },
    { "n_single_space", " ", 1 },
    { "n_string_1_surrogate_then_escape", "[\"\\uD800\\\"]", 11 },
    { "n_string_1_surrogate_then_escape_u", "[\"\\uD800\\u\"]", 12 },
    { "n_string_1_surrogate_then_escape_u1", "[\"\\uD800\\u1\"]", 13 },
    { "n_string_1_surrogate_then_escape_u1x", "[\"\\uD800\\u1x\"]", 14 },
    { "n_string_accentuated_char_no_quotes", "[√©]", 4 },
    { "n_string_backslash_00", "[\"\\\0\"]", 6 },
    { "n_string_escape_x", "[\"\\x00\"]", 8 },
    { "n_string_escaped_backslash_bad", "[\"\\\\\\\"]", 7 },
    { "n_string_escaped_ctrl_char_tab", "[\"\\	\"]", 6 },
    { "n_string_escaped_emoji", "[\"\\üåÄ\"]", 9 },
    { "n_string_incomplete_escape", "[\"\\\"]", 5 },
    { "n_string_incomplete_escaped_character", "[\"\\u00A\"]", 9 },
    { "n_string_incomplete_surrogate", "[\"\\uD834\\uDd\"]", 14 },
    { "n_string_incomplete_surrogate_escape_invalid", "[\"\\uD800\\uD800\\x\"]", 18 },
    { "n_string_invalid-utf-8-in-escape", "[\"\\uÂ\"]", 7 },
    { "n_string_invalid_backslash_esc", "[\"\\a\"]", 6 },
    { "n_string_invalid_unicode_escape", "[\"\\uqqqq\"]", 10 },
    { "n_string_invalid_utf8_after_escape", "[\"\\Â\"]", 6 },
    { "n_string_leading_uescaped_thinspace", "[\\u0020\"asd\"]", 13 },
    { "n_string_no_quotes_with_bad_escape", "[\\n]", 4 },
    { "n_string_single_doublequote", "\"", 1 },
    { "n_string_single_quote", "['single quote']", 16 },
    { "n_string_single_string_no_double_quotes", "abc", 3 },
    { "n_string_start_escape_unclosed", "[\"\\", 3 },
    { "n_string_unescaped_crtl_char", "[\"a\0a\"]", 7 },
    { "n_string_unescaped_newline", "[\"new\nline\"]", 12 },
    { "n_string_unescaped_tab", "[\"	\"]", 5 },
    { "n_string_unicode_CapitalU", "\"\\UA66D\"", 8 },
    { "n_string_with_trailing_garbage", "\"\"x", 3 },

    { "n_structure_U+2060_word_joined", "[‚Å†]", 5 },
    { "n_structure_UTF8_BOM_no_data", "Ôªø", 3 },
    { "n_structure_angle_bracket_.", "<.>", 3 },
    { "n_structure_angle_bracket_null", "[<null>]", 8 },
    { "n_structure_array_trailing_garbage", "[1]x", 4 },
    { "n_structure_array_with_extra_array_close", "[1]]", 4 },
    { "n_structure_array_with_unclosed_string", "[\"asd]", 6 },
    { "n_structure_ascii-unicode-identifier", "a√•", 3 },
    { "n_structure_capitalized_True", "[True]", 6 },
    { "n_structure_close_unopened_array", "1]", 2 },
    { "n_structure_comma_instead_of_closing_brace", "{\"x\": true,", 11 },
    { "n_structure_double_array", "[][]", 4 },
    { "n_structure_end_array", "]", 1 },
    { "n_structure_incomplete_UTF8_BOM", "Ôª{}", 4 },
    { "n_structure_lone-invalid-utf-8", "Â", 1 },
    { "n_structure_lone-open-bracket", "[", 1 },
    { "n_structure_no_data", "", 0 },
    { "n_structure_null-byte-outside-string", "[\0]", 3 },
    { "n_structure_number_with_trailing_garbage", "2@", 2 },
    { "n_structure_object_followed_by_closing_object", "{}}", 3 },
    { "n_structure_object_unclosed_no_value", "{\"\":", 4 },
    { "n_structure_object_with_comment", "{\"a\":/*comment*/\"b\"}", 20 },
    { "n_structure_object_with_trailing_garbage", "{\"a\": true} \"x\"", 15 },
    { "n_structure_open_array_apostrophe", "['", 2 },
    { "n_structure_open_array_comma", "[,", 2 },
    { "n_structure_open_array_open_object", "[{", 2 },
    { "n_structure_open_array_open_string", "[\"a", 3 },
    { "n_structure_open_array_string", "[\"a\"", 4 },
    { "n_structure_open_object", "{", 1 },
    { "n_structure_open_object_close_array", "{]", 2 },
    { "n_structure_open_object_comma", "{,", 2 },
    { "n_structure_open_object_open_array", "{[", 2 },
    { "n_structure_open_object_open_string", "{\"a", 3 },
    { "n_structure_open_object_string_with_apostrophes", "{'a'", 4 },
    { "n_structure_open_open", "[\"\\{[\"\\{[\"\\{[\"\\{", 16 },
    { "n_structure_single_eacute", "È", 1 },
    { "n_structure_single_star", "*", 1 },
    { "n_structure_trailing_#", "{\"a\":\"b\"}#{}", 12 },
    { "n_structure_uescaped_LF_before_string", "[\\u000A\"\"]", 10 },
    { "n_structure_unclosed_array", "[1", 2 },
    { "n_structure_unclosed_array_partial_null", "[ false, nul", 12 },
    { "n_structure_unclosed_array_unfinished_false", "[ true, fals", 12 },
    { "n_structure_unclosed_array_unfinished_true", "[ false, tru", 12 },
    { "n_structure_unclosed_object", "{\"asd\":\"asd\"", 12 },
    { "n_structure_unicode-identifier", "√•", 2 },
    { "n_structure_whitespace_U+2060_word_joiner", "[‚Å†]", 5 },
    { "n_structure_whitespace_formfeed", "[]", 3 },
    { "y_array_arraysWithSpaces", "[[]   ]", 7 },
    { "y_array_empty-string", "[\"\"]", 4 },
    { "y_array_empty", "[]", 2 },
    { "y_array_ending_with_newline", "[\"a\"]", 5 },
    { "y_array_false", "[false]", 7 },
    { "y_array_heterogeneous", "[null, 1, \"1\", {}]", 18 },
    { "y_array_null", "[null]", 6 },
    { "y_array_with_1_and_newline", "[1\n]", 4 },
    { "y_array_with_leading_space", " [1]", 4 },
    { "y_array_with_several_null", "[1,null,null,null,2]", 20 },
    { "y_array_with_trailing_space", "[2] ", 4 },
    { "y_number", "[123e65]", 8 },
    { "y_number_0e+1", "[0e+1]", 6 },
    { "y_number_0e1", "[0e1]", 5 },
    { "y_number_after_space", "[ 4]", 4 },
    { "y_number_double_close_to_zero", "[-0.000000000000000000000000000000000000000000000000000000000000000000000000000001]\n", 84 },
    { "y_number_int_with_exp", "[20e1]", 6 },
    { "y_number_minus_zero", "[-0]", 4 },
    { "y_number_negative_int", "[-123]", 6 },
    { "y_number_negative_one", "[-1]", 4 },
    { "y_number_negative_zero", "[-0]", 4 },
    { "y_number_real_capital_e", "[1E22]", 6 },
    { "y_number_real_capital_e_neg_exp", "[1E-2]", 6 },
    { "y_number_real_capital_e_pos_exp", "[1E+2]", 6 },
    { "y_number_real_exponent", "[123e45]", 8 },
    { "y_number_real_fraction_exponent", "[123.456e78]", 12 },
    { "y_number_real_neg_exp", "[1e-2]", 6 },
    { "y_number_real_pos_exponent", "[1e+2]", 6 },
    { "y_number_simple_int", "[123]", 5 },
    { "y_number_simple_real", "[123.456789]", 12 },
    { "y_object", "{\"asd\":\"sdf\", \"dfg\":\"fgh\"}", 26 },
    { "y_object_basic", "{\"asd\":\"sdf\"}", 13 },
    { "y_object_duplicated_key", "{\"a\":\"b\",\"a\":\"c\"}", 17 },
    { "y_object_duplicated_key_and_value", "{\"a\":\"b\",\"a\":\"b\"}", 17 },
    { "y_object_empty", "{}", 2 },
    { "y_object_empty_key", "{\"\":0}", 6 },
    { "y_object_escaped_null_in_key", "{\"foo\\u0000bar\": 42}", 20 },
    { "y_object_extreme_numbers", "{ \"min\": -1.0e+28, \"max\": 1.0e+28 }", 35 },
    { "y_object_long_strings", "{\"x\":[{\"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}], \"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}", 108 },
    { "y_object_simple", "{\"a\":[]}", 8 },
    { "y_object_string_unicode", "{\"title\":\"\\u041f\\u043e\\u043b\\u0442\\u043e\\u0440\\u0430 \\u0417\\u0435\\u043c\\u043b\\u0435\\u043a\\u043e\\u043f\\u0430\" }", 110 },
    { "y_object_with_newlines", "{\n\"a\": \"b\"\n}", 12 },
    { "y_string_1_2_3_bytes_UTF-8_sequences", "[\"\\u0060\\u012a\\u12AB\"]", 22 },
    { "y_string_accepted_surrogate_pair", "[\"\\uD801\\udc37\"]", 16 },
    { "y_string_accepted_surrogate_pairs", "[\"\\ud83d\\ude39\\ud83d\\udc8d\"]", 28 },
    { "y_string_allowed_escapes", "[\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"]", 20 },
    { "y_string_backslash_and_u_escaped_zero", "[\"\\\\u0000\"]", 11 },
    { "y_string_backslash_doublequotes", "[\"\\\"\"]", 6 },
    { "y_string_comments", "[\"a/*b*/c/*d//e\"]", 17 },
    { "y_string_double_escape_a", "[\"\\\\a\"]", 7 },
    { "y_string_double_escape_n", "[\"\\\\n\"]", 7 },
    { "y_string_escaped_control_character", "[\"\\u0012\"]", 10 },
    { "y_string_escaped_noncharacter", "[\"\\uFFFF\"]", 10 },
    { "y_string_in_array", "[\"asd\"]", 7 },
    { "y_string_in_array_with_leading_space", "[ \"asd\"]", 8 },
    { "y_string_last_surrogates_1_and_2", "[\"\\uDBFF\\uDFFF\"]", 16 },
    { "y_string_nbsp_uescaped", "[\"new\\u00A0line\"]", 17 },
    { "y_string_nonCharacterInUTF-8_U+10FFFF", "[\"Ùèøø\"]", 8 },
    { "y_string_nonCharacterInUTF-8_U+1FFFF", "[\"õøø\"]", 8 },
    { "y_string_nonCharacterInUTF-8_U+FFFF", "[\"Ôøø\"]", 7 },
    { "y_string_null_escape", "[\"\\u0000\"]", 10 },
    { "y_string_one-byte-utf-8", "[\"\\u002c\"]", 10 },
    { "y_string_pi", "[\"œÄ\"]", 6 },
    { "y_string_simple_ascii", "[\"asd \"]", 8 },
    { "y_string_space", "\" \"", 3 },
    { "y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF", "[\"\\uD834\\uDd1e\"]", 16 },
    { "y_string_three-byte-utf-8", "[\"\\u0821\"]", 10 },
    { "y_string_two-byte-utf-8", "[\"\\u0123\"]", 10 },
    { "y_string_u+2028_line_sep", "[\"‚Ä®\"]", 7 },
    { "y_string_u+2029_par_sep", "[\"‚Ä©\"]", 7 },
    { "y_string_uEscape", "[\"\\u0061\\u30af\\u30EA\\u30b9\"]", 28 },
    { "y_string_uescaped_newline", "[\"new\\u000Aline\"]", 17 },
    { "y_string_unescaped_char_delete", "[\"\"]", 5 },
    { "y_string_unicode", "[\"\\uA66D\"]", 10 },
    { "y_string_unicodeEscapedBackslash", "[\"\\u005C\"]", 10 },
    { "y_string_unicode_2", "[\"‚çÇ„à¥‚çÇ\"]", 13 },
    { "y_string_unicode_U+10FFFE_nonchar", "[\"\\uDBFF\\uDFFE\"]", 16 },
    { "y_string_unicode_U+1FFFE_nonchar", "[\"\\uD83F\\uDFFE\"]", 16 },
    { "y_string_unicode_U+200B_ZERO_WIDTH_SPACE", "[\"\\u200B\"]", 10 },
    { "y_string_unicode_U+2064_invisible_plus", "[\"\\u2064\"]", 10 },
    { "y_string_unicode_U+FDD0_nonchar", "[\"\\uFDD0\"]", 10 },
    { "y_string_unicode_U+FFFE_nonchar", "[\"\\uFFFE\"]", 10 },
    { "y_string_unicode_escaped_double_quote", "[\"\\u0022\"]", 10 },
    { "y_string_utf8", "[\"‚Ç¨ùÑû\"]", 11 },
    { "y_string_with_del_character", "[\"aa\"]", 7 },
    { "y_structure_lonely_false", "false", 5 },
    { "y_structure_lonely_int", "42", 2 },
    { "y_structure_lonely_negative_real", "-0.1", 4 },
    { "y_structure_lonely_null", "null", 4 },
    { "y_structure_lonely_string", "\"asd\"", 5 },
    { "y_structure_lonely_true", "true", 4 },
    { "y_structure_string_empty", "\"\"", 2 },
    { "y_structure_trailing_newline", "[\"a\"]\n", 6 },
    { "y_structure_true_in_array", "[true]", 6 },
    { "y_structure_whitespace_array", " [] ", 4 },
};


/*
 * If any tests fail, this is set to 1.
 */
static int mxjson_test_return_code;


/**
 * Perform a test by parsing some JSON
 *
 * The first character of test_name indicates the expected result:
 *  - 'y' : Valid JSON, expected to parse successfully.
 *  - 'n' : Invalid JSON, expected to be rejected.
 *  - 'i' : Implementation dependent (expected to parse successfully).
 *  - 'u' : Implementation dependent (expected to be rejected).
 */
static void
mxjson_test (char *test_name, mxjson_parser_t *p, mxstr_t json)
{
    bool ok;
    bool fail;

    ok = mxjson_parse(p, json);

    switch (test_name[0]) {
    case 'i':
    case 'y':
        fail = !ok;
        break;

    case 'n':
    case 'u':
        fail = ok;
        break;

    default:
        fail = true;
        break;
    }

    printf("%s: %-60s %s\n", fail ? "FAIL" : "PASS", test_name,
           ok ? "Valid" : (p->idx >= p->count) ? "Errored" : "Rejected");

    mxjson_test_return_code |= fail;
}

/**
 * Number of tokens to initially allocate, sufficient for any of the
 * testcases in testcases[].
 */
#define TOKEN_COUNT 8


/**
 * Test resize function that fails to allocate more tokens.
 */
static bool
mxjson_test_resize (mxjson_parser_t *p, mxjson_idx_t size_hint)
{
    assert(size_hint == 0 || size_hint > p->count);

    return (size_hint == 0);
}


int main (void)
{
    mxjson_token_t  tokens[TOKEN_COUNT];
    mxjson_parser_t p;
    mxbuf_t         buffer;
    mxstr_t         json;
    unsigned int    i;

    mxjson_init(&p, TOKEN_COUNT, tokens, NULL);

    for (i = 0; i < mxarray_size(testcases); i++) {
        json = mxstr(testcases[i].json, testcases[i].len);
        mxjson_test(testcases[i].test_name, &p, json);
    }

    mxbuf_create(&buffer, NULL, 0);
    mxbuf_write_chars(&buffer, '[', 500);
    mxbuf_write_chars(&buffer, ']', 500);
    mxjson_test("n_insufficient_tokens", &p, mxbuf_str(&buffer));
    mxjson_free(&p);

    mxjson_init(&p, TOKEN_COUNT, tokens, mxjson_test_resize);
    mxjson_test("n_token_resize_fails", &p, mxbuf_str(&buffer));
    mxjson_free(&p);

    mxjson_init(&p, TOKEN_COUNT, NULL, mxjson_test_resize);
    mxjson_test("n_initial_token_allocation_fails", &p, mxbuf_str(&buffer));
    mxjson_free(&p);

    mxjson_init(&p, 0, NULL, NULL);
    mxjson_test("n_no_token_memory", &p, mxbuf_str(&buffer));
    mxjson_free(&p);

    /*
     * Reinitialize to allow token array resizing.
     */
    mxjson_init(&p, TOKEN_COUNT, NULL, mxjson_resize);
    mxjson_test("i_structure_500_nested_arrays", &p, mxbuf_str(&buffer));

    mxbuf_reset(&buffer);
    mxbuf_write_chars(&buffer, '[', 100000);
    mxjson_test("n_structure_100000_opening_arrays", &p, mxbuf_str(&buffer));

    mxbuf_reset(&buffer);
    for (i = 0; i < 50000; i++) {
        mxbuf_write(&buffer, mxstr_literal("[{\"\":"));
    }
    mxbuf_putc(&buffer, '\n');
    mxjson_test("n_structure_open_array_object", &p, mxbuf_str(&buffer));
    mxbuf_free(&buffer);

    mxjson_free(&p);

    return mxjson_test_return_code;
}
