import unittest

from model import ValidationError, recommend

VALID = {"D": 8, "W": 24, "F": .65, "P_read": .35, "P_write": .2,
         "P_mkdir": .1, "P_ls": .15, "P_mv": .1, "P_find": .1,
         "Dist": "uniform", "Zipf_Alpha": 1.2, "Loc": .75}


class RecommendationTests(unittest.TestCase):
    def test_returns_all_candidates_and_best(self):
        result = recommend(VALID)
        self.assertIn(result["best"], {"A", "B", "C"})
        self.assertEqual(set(result["predictions"]), {"A", "B", "C"})

    def test_rejects_invalid_probability_sum(self):
        with self.assertRaises(ValidationError) as context:
            recommend({**VALID, "P_read": .5})
        self.assertIn("probabilities", context.exception.errors)

    def test_rejects_dimensions_above_limits(self):
        for key, value in (("D", 21), ("W", 71)):
            with self.subTest(key=key):
                with self.assertRaises(ValidationError) as context:
                    recommend({**VALID, key: value})
                self.assertIn(key, context.exception.errors)

    def test_zipf_requires_valid_alpha(self):
        with self.assertRaises(ValidationError) as context:
            recommend({**VALID, "Dist": "zipf", "Zipf_Alpha": 4})
        self.assertIn("Zipf_Alpha", context.exception.errors)


if __name__ == "__main__":
    unittest.main()
