import os
from math import isfinite, prod

import joblib
import numpy as np
import pandas as pd

PROBABILITY_KEYS = ("P_read", "P_write", "P_mkdir", "P_ls", "P_mv", "P_find")

STRATEGY_WEIGHTS = {
    "balanced": {
        "mean_latency": 0.35,
        "p99_latency": 0.20,
        "memory": 0.20,
        "throughput": 0.25,
    },
    "memory": {
        "mean_latency": 0.10,
        "p99_latency": 0.10,
        "memory": 0.70,
        "throughput": 0.10,
    },
    "latency": {
        "mean_latency": 0.70,
        "p99_latency": 0.10,
        "memory": 0.10,
        "throughput": 0.10,
    },
    "p99": {
        "mean_latency": 0.10,
        "p99_latency": 0.70,
        "memory": 0.10,
        "throughput": 0.10,
    },
    "throughput": {
        "mean_latency": 0.10,
        "p99_latency": 0.10,
        "memory": 0.10,
        "throughput": 0.70,
    },
}


class ValidationError(ValueError):
    def __init__(self, errors):
        super().__init__("Invalid recommendation parameters")
        self.errors = errors


def validate_params(raw):
    errors, params = {}, {}

    def number(key, low, high, integer=False):
        try:
            value = int(raw[key]) if integer else float(raw[key])
            if not isfinite(float(value)) or not low <= value <= high:
                raise ValueError
            params[key] = value
        except (KeyError, TypeError, ValueError):
            errors[key] = f"Введите значение от {low:g} до {high:g}"

    number("D", 1, 20, True)
    number("W", 1, 70, True)
    number("F", 0.01, 1)
    number("Loc", 0, 1)
    for key in PROBABILITY_KEYS:
        number(key, 0, 1)

    dist = str(raw.get("Dist", "")).lower()
    if dist not in {"uniform", "zipf"}:
        errors["Dist"] = "Выберите закон распределения"
    params["Dist"] = dist

    strategy = str(raw.get("Strategy", "balanced")).lower()
    if strategy not in STRATEGY_WEIGHTS:
        errors["Strategy"] = "Выберите доступную стратегию"
    params["Strategy"] = strategy

    if dist == "zipf":
        number("Zipf_Alpha", 0.1, 3)
    else:
        params["Zipf_Alpha"] = None

    if not any(key in errors for key in PROBABILITY_KEYS):
        total = sum(float(params[key]) for key in PROBABILITY_KEYS)
        if abs(total - 1) > 1e-4:
            errors["probabilities"] = (
                f"Сумма вероятностей должна быть 1, сейчас {total:.3f}"
            )
    if errors:
        raise ValidationError(errors)
    return params


def load_models(base_path="model"):
    models = {}
    for system in ["A", "B", "C"]:
        models[system] = {
            "pipeline": joblib.load(
                f"{base_path}/{system}/random_forest_pipeline.joblib"
            ),
            "scaler_X": joblib.load(f"{base_path}/{system}/scaler_X.joblib"),
            "scaler_Y": joblib.load(f"{base_path}/{system}/scaler_Y.joblib"),
        }
    return models


def estimate_nodes(depth, width, fill):
    if width == 1:
        return fill * (depth + 1)
    return fill * (width ** (depth + 1) - 1) / (width - 1)


def build_features(raw):
    D, W, F = raw["D"], raw["W"], raw["F"]
    locality = raw["locality"]
    distribution = raw["distribution"]
    zipf_s = 0.0 if distribution == "uniform" else raw["zipf_s"]
    p_read, p_ls, p_find = raw["p_read"], raw["p_ls"], raw["p_find"]
    p_write, p_mv = raw["p_write"], raw["p_mv"]

    estimated_nodes = estimate_nodes(D, W, F)
    logD, logW = np.log(D), np.log(W)
    log_estimated_nodes = np.log1p(estimated_nodes)
    zipf_depth_interaction = zipf_s * logD
    mutation_ops = p_write + p_mv
    distribution_zipf = 1 if distribution == "zipf" else 0
    high_read_uniform = int((p_read > 0.6) and (distribution_zipf == 0))
    nodes_x_read = estimated_nodes * p_read
    nodes_x_mutation = estimated_nodes * mutation_ops

    return {
        "F": F,
        "D": D,
        "W": W,
        "locality": locality,
        "p_read": p_read,
        "p_ls": p_ls,
        "p_find": p_find,
        "p_write": p_write,
        "p_mv": p_mv,
        "zipf_s": zipf_s,
        "estimated_nodes": estimated_nodes,
        "logD": logD,
        "logW": logW,
        "log_estimated_nodes": log_estimated_nodes,
        "zipf_depth_interaction": zipf_depth_interaction,
        "mutation_ops": mutation_ops,
        "distribution_zipf": distribution_zipf,
        "high_read_uniform": high_read_uniform,
        "nodes_x_read": nodes_x_read,
        "nodes_x_mutation": nodes_x_mutation,
    }


SYSTEM_FEATURES = {
    "A": [
        "nodes_x_read",
        "nodes_x_mutation",
        "log_estimated_nodes",
        "high_read_uniform",
        "estimated_nodes",
        "F",
        "logD",
        "logW",
        "locality",
        "zipf_s",
        "zipf_depth_interaction",
        "p_read",
        "distribution_zipf",
    ],
    "B": [
        "estimated_nodes",
        "logD",
        "logW",
        "locality",
        "p_ls",
        "p_find",
        "p_write",
        "p_mv",
        "zipf_s",
        "zipf_depth_interaction",
        "p_read",
        "log_estimated_nodes",
    ],
    "C": [
        "estimated_nodes",
        "logW",
        "locality",
        "p_read",
        "p_ls",
        "p_find",
        "p_write",
        "p_mv",
        "log_estimated_nodes",
        "nodes_x_read",
        "nodes_x_mutation",
    ],
}

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BASE_MODEL_PATH = os.path.join(SCRIPT_DIR, "model")
MODELS = load_models(base_path=BASE_MODEL_PATH)

ORDERED_TARGETS = [
    "memory_usage_bytes",
    "avg_latency_us",
    "p99_latency_us",
    "throughput_ops_sec",
]


def predict_metrics(params):
    raw_input = {
        "D": params.get("D"),
        "W": params.get("W"),
        "F": params.get("F"),
        "locality": params.get("Loc", 1),
        "distribution": params.get("Dist", "uniform"),
        "zipf_s": (
            params.get("Zipf_Alpha", 0.0)
            if params.get("Zipf_Alpha") is not None
            else 0.0
        ),
        "p_ls": params.get("P_ls", 0.0),
        "p_find": params.get("P_find", 0.0),
        "p_mv": params.get("P_mv", 0.0),
        "p_read": params.get("P_read", 0.0),
        "p_write": params.get("P_write", 0.0),
        "p_mkdir": params.get("P_mkdir", 0.0),
    }

    features_dict = build_features(raw_input)

    df_X = pd.DataFrame([features_dict])

    result = {}

    for system in ["A", "B", "C"]:
        sys_model = MODELS[system]

        X_system = df_X[SYSTEM_FEATURES[system]]
        X_scaled = sys_model["scaler_X"].transform(X_system)

        pred_scaled = sys_model["pipeline"].predict(X_scaled)

        if pred_scaled.ndim == 1:
            pred_scaled = pred_scaled.reshape(1, -1)

        pred_original = sys_model["scaler_Y"].inverse_transform(pred_scaled)[0]

        targets_dict = dict(zip(ORDERED_TARGETS, pred_original))

        result[system] = {
            "mean_latency": float(targets_dict["avg_latency_us"]),
            "p99_latency": float(targets_dict["p99_latency_us"]),
            "memory": float(targets_dict["memory_usage_bytes"]),
            "throughput": float(targets_dict["throughput_ops_sec"]),
        }

    return result


def select_best_approach(predictions, strategy="balanced"):
    weights = STRATEGY_WEIGHTS[strategy]
    candidates = list(predictions)
    bounds = {
        key: (
            min(predictions[candidate][key] for candidate in candidates),
            max(predictions[candidate][key] for candidate in candidates),
        )
        for key in weights
    }

    def normalize(value, key):
        minimum, maximum = bounds[key]
        if maximum == minimum:
            return 1.0
        if key == "throughput":
            score = (value - minimum) / (maximum - minimum)
        else:
            score = (maximum - value) / (maximum - minimum)
        return max(score, 0.01)

    scores = {
        candidate: prod(
            normalize(predictions[candidate][key], key) ** weight
            for key, weight in weights.items()
        )
        for candidate in candidates
    }
    return max(scores, key=scores.get), scores


def recommend(raw):
    params = validate_params(raw)
    predictions = predict_metrics(params)
    best, scores = select_best_approach(predictions, params["Strategy"])
    return {
        "best": best,
        "scores": scores,
        "predictions": predictions,
        "strategy": params["Strategy"],
        "estimated_nodes": max(
            int(estimate_nodes(params["D"], params["W"], params["F"])), 1
        ),
    }
