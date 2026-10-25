"""Regression tests for the documentation checker; no app build required."""

import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("check_docs", Path(__file__).resolve().parents[1] / "check-docs.py")
DOCS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DOCS)


class DocumentationChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.put("docs/README.md", "# Documentation\n")

    def put(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def check(self):
        return DOCS.check(self.root)[0]

    def test_valid_relative_link_and_anchor(self):
        self.put("docs/README.md", "[Topic](components/topic.md#contract)\n")
        self.put("docs/components/topic.md", "# Topic\n## Contract\n[Back](../README.md)\n")
        self.assertEqual([], self.check())

    def test_missing_file_and_heading(self):
        self.put("docs/README.md", "[Missing](missing.md)\n[Bad heading](#absent)\n")
        errors = self.check()
        self.assertTrue(any("missing target" in error for error in errors))
        self.assertTrue(any("missing anchor" in error for error in errors))

    def test_fragments_unicode_and_duplicate_headings(self):
        self.put("docs/README.md", "# Über `MIDI`!\n## Part\n## Part\n[Self](#über-midi)\n[Again](#part-1)\n<a id=\"explicit\"></a>\n[HTML](#explicit)\n")
        self.assertEqual([], self.check())

    def test_setext_headings(self):
        self.assertIn("contract", DOCS.anchors("Contract\n--------\n"))

    def test_heading_suffix_collisions(self):
        ids = DOCS.anchors("# Part\n# Part\n# Part-1\n")
        self.assertEqual({"part", "part-1", "part-1-1"}, ids)

    def test_code_comments_and_external_links_are_not_checked(self):
        self.put("docs/README.md", "```markdown\n[Fake](absent.md)\n```\n~~~\n[Fake](absent.md)\n~~~\n    [Indented](absent.md)\n`[Inline](absent.md)`\n<!-- [Hidden](absent.md) -->\n[Remote](https://example.invalid/nope#missing)\n[Mail](mailto:example@example.invalid)\n")
        self.assertEqual([], self.check())

    def test_code_headings_do_not_create_anchors(self):
        self.put("docs/README.md", "```\n# Fake\n```\n[Bad](#fake)\n")
        self.assertTrue(any("missing anchor" in error for error in self.check()))

    def test_encoded_and_balanced_paths_images_titles(self):
        self.put("docs/README.md", '[Space](<page space.md> "title")\n[Encoded](page%20space.md)\n[Paren](page(foo).md)\n![Image](image.png "title")\n')
        self.put("docs/page space.md", "# Space\n")
        self.put("docs/page(foo).md", "# Parentheses\n")
        self.put("docs/image.png", "not decoded by this checker")
        self.assertEqual([], self.check())

    def test_list_links_are_checked_with_original_line_numbers(self):
        text = ('- Parent\n'
                '    - [Nested](nested.md)\n'
                '        - [Deeper](deep.md)\n'
                '\n'
                '1. Ordered parent\n'
                '\n'
                '    [Continuation](continued.md)\n')
        self.put("docs/README.md", text)
        self.assertEqual([(2, "nested.md"), (3, "deep.md"), (7, "continued.md")],
                         list(DOCS.links(text)))
        self.assertEqual(3, sum("missing target" in error for error in self.check()))

    def test_list_links_provide_index_coverage(self):
        self.put("docs/README.md", "- Topics\n    - [Topic](topic.md#contract)\n")
        self.put("docs/topic.md", "# Topic\n## Contract\n")
        self.assertEqual([], self.check())

    def test_code_inside_lists_is_not_checked(self):
        text = ('- Parent\n\n'
                '      [Indented code](absent.md)\n\n'
                '    - Child\n\n'
                '          [Nested code](absent.md)\n\n'
                '      ```markdown\n'
                '      [Fenced code](absent.md)\n'
                '      ```\n'
                '      [Real](real.md)\n\n'
                'Outside the list\n\n'
                '    [Standalone code](absent.md)\n')
        self.put("docs/README.md", text)
        self.put("docs/real.md", "# Real\n")
        self.assertEqual([(12, "real.md")], list(DOCS.links(text)))
        self.assertEqual([], self.check())

    def test_parentheses_in_quoted_link_titles(self):
        for title in ('"API (legacy"', "'API (legacy'", '"API )legacy"',
                      '"API \\\"(legacy"'):
            with self.subTest(title=title):
                text = f'[Broken](absent.md {title})\n'
                self.put("docs/README.md", text)
                self.assertEqual([(1, "absent.md")], list(DOCS.links(text)))
                self.assertTrue(any("missing target" in error for error in self.check()))

    def test_quoted_titles_with_balanced_and_angle_destinations(self):
        text = ('[Parentheses](page(foo).md "API (legacy")\n'
                '![Image](<image (legacy).png> \'API )legacy\')\n'
                '[Apostrophe](page\'s.md "API (legacy")\n')
        self.put("docs/README.md", text)
        self.put("docs/page(foo).md", "# Page\n")
        self.put("docs/image (legacy).png", "image fixture")
        self.put("docs/page's.md", "# Apostrophe\n")
        self.assertEqual([(1, "page(foo).md"), (2, "image (legacy).png"),
                          (3, "page's.md")], list(DOCS.links(text)))
        self.assertEqual([], self.check())

    def test_reference_links(self):
        self.put("docs/README.md", "[Topic][ref]\n[ref][]\n[ref]\n[ref]: topic.md#topic\n")
        self.put("docs/topic.md", "# Topic\n")
        self.assertEqual([], self.check())

    def test_undefined_reference_is_an_error(self):
        self.put("docs/README.md", "[Topic][missing]\n")
        self.assertTrue(any("undefined-reference" in error for error in self.check()))

    def test_malformed_url_is_reported_without_crashing(self):
        self.put("docs/README.md", "[Malformed](https://[broken)\n")
        self.assertTrue(any("malformed link" in error for error in self.check()))

    def test_bulleted_redirect_metadata(self):
        self.put("docs/old.md", "# Old\n- Type: redirect\n[Current](README.md)\n")
        self.assertEqual([], self.check())

    def test_unindexed_current_page_fails(self):
        self.put("docs/current.md", "# Current\n")
        self.assertTrue(any("not reachable" in error for error in self.check()))

    def test_archive_legacy_and_redirect_are_exempt_only_from_index(self):
        self.put("docs/archive/old.md", "# Historical\n")
        self.put("docs/changes/old.md", "# Legacy history\n")
        self.put("docs/old-path.md", "# Old\nType: redirect\n[Current](README.md)\n")
        self.assertEqual([], self.check())
        self.put("docs/archive/old.md", "[Broken](missing.md)\n")
        self.assertTrue(any("missing target" in error for error in self.check()))

    def test_two_step_navigation_and_history_not_counted(self):
        self.put("docs/README.md", "[Plans](plans/README.md)\n[History](archive/README.md)\n")
        self.put("docs/plans/README.md", "[Plan](active.md)\n")
        self.put("docs/plans/active.md", "# Active\n")
        self.put("docs/archive/README.md", "[Hidden current](../current.md)\n")
        self.put("docs/current.md", "# Current\n")
        self.assertEqual(1, len(self.check()))
        self.assertIn("docs/current.md", self.check()[0])

    def test_link_cannot_escape_repository(self):
        self.put("docs/README.md", "[Outside](../../outside.md)\n")
        self.assertTrue(any("leaves repository" in error for error in self.check()))

    def test_root_pages_are_checked(self):
        self.put("README.md", "[Missing](missing.md)\n")
        self.assertTrue(any("README.md:1: missing" in error for error in self.check()))

    def test_repository_absolute_path_and_encoded_fragment(self):
        self.put("docs/README.md", "[Root](/docs/topic.md#%C3%BCber)\n")
        self.put("docs/topic.md", "# Über\n")
        self.assertEqual([], self.check())


if __name__ == "__main__":
    unittest.main()
