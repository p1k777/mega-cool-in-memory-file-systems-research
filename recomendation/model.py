from math import isfinite

PROBABILITY_KEYS = ("P_read", "P_write", "P_mkdir", "P_ls", "P_mv", "P_find")


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


def predict_metrics(params):
    depth, width, fill = params["D"], params["W"], params["F"]
    p_ls, p_mv, p_find = params["P_ls"], params["P_mv"], params["P_find"]
    nodes = max(int(width * depth * fill * 100), 500)
    zipf_bonus = 1.0
    if params["Dist"] == "zipf" and params["Zipf_Alpha"] is not None:
        zipf_bonus = max(0.4, 1.0 - params["Zipf_Alpha"] * 0.25)

    latency_a = (4 + depth * 1.8 + p_ls * 45 + p_find * 110) * zipf_bonus
    latency_b = (1.8 + p_mv * depth * 180 + p_find * 30) * (
        zipf_bonus * 0.8 if params["Dist"] == "zipf" else 1
    )
    latency_c = 2.5 + p_ls * nodes * 0.02 + p_find * nodes * 0.05

    def metrics(latency, memory_per_node, p99_factor):
        return {
            "mean_latency": latency,
            "p99_latency": latency * p99_factor,
            "memory": float(nodes * memory_per_node),
            "throughput": 1_000_000 / latency,
        }

    return {
        "A": metrics(latency_a, 48, 1.9),
        "B": metrics(latency_b, 160, 1.25),
        "C": metrics(latency_c, 112, 1.4),
    }


def select_best_approach(predictions):
    weights = (0.35, 0.20, 0.20, 0.25)
    candidates = list(predictions)
    keys = ("mean_latency", "p99_latency", "memory", "throughput")
    bounds = {
        key: (
            min(predictions[candidate][key] for candidate in candidates),
            max(predictions[candidate][key] for candidate in candidates),
        )
        for key in keys
    }

    def normalize(value, key, cost):
        minimum, maximum = bounds[key]
        if maximum == minimum:
            return 1.0
        score = (
            (maximum - value) if cost else (value - minimum)
        ) / (maximum - minimum)
        return max(score, 0.01)

    scores = {}
    for candidate in candidates:
        metric = predictions[candidate]
        normalized = (
            normalize(metric["mean_latency"], "mean_latency", True),
            normalize(metric["p99_latency"], "p99_latency", True),
            normalize(metric["memory"], "memory", True),
            normalize(metric["throughput"], "throughput", False),
        )
        scores[candidate] = sum(
            weight * score for weight, score in zip(weights, normalized)
        )
    return max(scores, key=scores.get), scores


def recommend(raw):
    params = validate_params(raw)
    predictions = predict_metrics(params)
    best, scores = select_best_approach(predictions)
    return {
        "best": best,
        "scores": scores,
        "predictions": predictions,
        "estimated_nodes": max(
            int(params["W"] * params["D"] * params["F"] * 100), 500
        ),
    }