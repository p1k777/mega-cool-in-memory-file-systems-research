import unittest

from model import STRATEGY_WEIGHTS, ValidationError, recommend, select_best_approach

VALID = {
    "D": 8,
    "W": 24,
    "F": 0.65,
    "P_read": 0.35,
    "P_write": 0.2,
    "P_mkdir": 0.1,
    "P_ls": 0.15,
    "P_mv": 0.1,
    "P_find": 0.1,
    "Dist": "uniform",
    "Zipf_Alpha": 1.2,
    "Loc": 0.75,
    "Strategy": "balanced",
}


class RecommendationTests(unittest.TestCase):
    def test_returns_all_candidates_and_best(self):
        result = recommend(VALID)
        self.assertIn(result["best"], {"A", "B", "C"})
        self.assertEqual(set(result["predictions"]), {"A", "B", "C"})
        self.assertEqual(result["strategy"], "balanced")

    def test_geometric_score_penalizes_a_single_failed_metric(self):
        predictions = {
            "A": {
                "mean_latency": 1,
                "p99_latency": 3,
                "memory": 1,
                "throughput": 3,
            },
            "B": {
                "mean_latency": 2,
                "p99_latency": 2,
                "memory": 2,
                "throughput": 2,
            },
            "C": {
                "mean_latency": 3,
                "p99_latency": 1,
                "memory": 3,
                "throughput": 1,
            },
        }
        best, scores = select_best_approach(predictions, "balanced")
        self.assertEqual(best, "B")
        self.assertGreater(scores["B"], scores["A"])

    def test_specialized_strategies_keep_secondary_metrics(self):
        for strategy in ("memory", "latency", "throughput"):
            with self.subTest(strategy=strategy):
                weights = STRATEGY_WEIGHTS[strategy]
                self.assertAlmostEqual(sum(weights.values()), 1.0)
                self.assertTrue(all(weight > 0 for weight in weights.values()))

        self.assertEqual(
            STRATEGY_WEIGHTS["latency"],
            {
                "mean_latency": 0.60,
                "p99_latency": 0.30,
                "memory": 0.05,
                "throughput": 0.05,
            },
        )
        self.assertAlmostEqual(STRATEGY_WEIGHTS["memory"]["memory"], 0.70)
        self.assertAlmostEqual(
            STRATEGY_WEIGHTS["throughput"]["throughput"], 0.70
        )

    def test_metric_strategies_select_their_best_candidate(self):
        predictions = {
            "A": {
                "mean_latency": 1,
                "p99_latency": 2,
                "memory": 200,
                "throughput": 200,
            },
            "B": {
                "mean_latency": 2,
                "p99_latency": 1,
                "memory": 200,
                "throughput": 200,
            },
            "C": {
                "mean_latency": 3,
                "p99_latency": 3,
                "memory": 100,
                "throughput": 300,
            },
        }
        expected = {
            "latency": "A",
            "memory": "C",
            "throughput": "C",
        }
        for strategy, candidate in expected.items():
            with self.subTest(strategy=strategy):
                best, _ = select_best_approach(predictions, strategy)
                self.assertEqual(best, candidate)

    def test_rejects_unknown_strategy(self):
        with self.assertRaises(ValidationError) as context:
            recommend({**VALID, "Strategy": "unknown"})
        self.assertIn("Strategy", context.exception.errors)

    def test_rejects_removed_p99_strategy(self):
        with self.assertRaises(ValidationError) as context:
            recommend({**VALID, "Strategy": "p99"})
        self.assertIn("Strategy", context.exception.errors)

    def test_rejects_invalid_probability_sum(self):
        with self.assertRaises(ValidationError) as context:
            recommend({**VALID, "P_read": 0.5})
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
