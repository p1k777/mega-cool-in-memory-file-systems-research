const form = document.querySelector("#form");
const results = document.querySelector("#results");
const probabilities = [...form.querySelectorAll('input[name^="P_"]')];
const percentageInputs = [...form.querySelectorAll(".prob-number")];
const total = document.querySelector("#total");
const probabilityError = document.querySelector("#prob-error");
const locality = form.elements.Loc;
const localityNumber = form.querySelector(".locality-number");
const alpha = document.querySelector("#alpha-field");
const button = form.querySelector(".submit");
const number = new Intl.NumberFormat("ru-RU", { maximumFractionDigits: 2 });
const integer = new Intl.NumberFormat("ru-RU", { maximumFractionDigits: 0 });

function updateProbabilities() {
    probabilities.forEach((input, index) => {
        if (document.activeElement !== percentageInputs[index]) {
            percentageInputs[index].value = Math.round(
                Number(input.value) * 100,
            );
        }
    });
    const sum = probabilities.reduce(
        (value, input) => value + Number(input.value),
        0,
    );
    const valid = Math.abs(sum - 1) < 0.0001;
    total.value = Math.round(sum * 100) + "%";
    total.classList.toggle("invalid", !valid);
    probabilityError.textContent = valid
        ? ""
        : "Сумма должна составлять ровно 100%";
    return valid;
}

probabilities.forEach((input) =>
    input.addEventListener("input", updateProbabilities),
);
percentageInputs.forEach((input, index) => {
    input.addEventListener("input", () => {
        const percent = Math.max(0, Math.min(100, Number(input.value) || 0));
        probabilities[index].value = percent / 100;
        updateProbabilities();
    });
    input.addEventListener("blur", updateProbabilities);
});
locality.addEventListener("input", () => {
    localityNumber.value = Math.round(locality.value * 100);
});
localityNumber.addEventListener("input", () => {
    const percent = Math.max(
        0,
        Math.min(100, Number(localityNumber.value) || 0),
    );
    locality.value = percent / 100;
});
localityNumber.addEventListener("blur", () => {
    localityNumber.value = Math.round(locality.value * 100);
});
form.elements.Dist.forEach((input) => {
    input.addEventListener("change", () => {
        alpha.classList.toggle(
            "visible",
            input.value === "zipf" && input.checked,
        );
    });
});

document.querySelector("#balance").addEventListener("click", () => {
    const sum = probabilities.reduce(
        (value, input) => value + Number(input.value),
        0,
    );
    let used = 0;
    probabilities.forEach((input, index) => {
        const value =
            sum === 0
                ? 1 / probabilities.length
                : index === probabilities.length - 1
                  ? 1 - used
                  : Math.round((Number(input.value) / sum) * 100) / 100;
        input.value = Math.max(0, Math.min(1, value)).toFixed(2);
        used += Number(input.value);
    });
    updateProbabilities();
});

function payload() {
    return Object.fromEntries(
        [...new FormData(form)].map(([key, value]) => [
            key,
            key === "Dist" ? value : Number(value),
        ]),
    );
}

function metricRow(label, value, width) {
    return (
        '<div class="metric"><span>' +
        label +
        "</span><strong>" +
        value +
        '</strong><i style="--width:' +
        width +
        '%"></i></div>'
    );
}

function render(data) {
    const best = data.predictions[data.best];
    const values = (key) =>
        Object.values(data.predictions).map((item) => item[key]);
    const quality = (key, value, inverse = true) => {
        const all = values(key);
        const min = Math.min(...all);
        const max = Math.max(...all);
        if (min === max) return 100;
        const position = inverse
            ? (max - value) / (max - min)
            : (value - min) / (max - min);
        return Math.round(24 + 76 * position);
    };
    const formatMemory = (bytes) =>
        bytes < 1048576
            ? integer.format(bytes / 1024) + " КБ"
            : number.format(bytes / 1048576) + " МБ";
    const memory = formatMemory(best.memory);
    const approaches = Object.entries(data.predictions)
        .sort((left, right) => data.scores[right[0]] - data.scores[left[0]])
        .map(
            ([name, metric]) =>
                '<div class="approach ' +
                (name === data.best ? "best" : "") +
                '">' +
                "<span>" +
                name +
                "</span><div><b>" +
                (name === data.best
                    ? "Лучшее соответствие"
                    : "Альтернативный подход") +
                "</b><small><span>Задержка<strong>" +
                number.format(metric.mean_latency) +
                " мкс</strong></span>" +
                "<span>Память<strong>" +
                formatMemory(metric.memory) +
                "</strong></span>" +
                "<span>Скорость<strong>" +
                integer.format(metric.throughput) +
                " оп/с</strong></span></small></div><strong>" +
                Math.round(data.scores[name] * 100) +
                "%</strong></div>",
        )
        .join("");

    results.innerHTML =
        '<div class="result"><div class="result-top"><p class="eyebrow">Рекомендация модели</p>' +
        '<div class="winner"><h2>Выбирайте подход ' +
        data.best +
        '</h2><div class="badge">' +
        data.best +
        '</div></div><p class="result-lead">Лучший баланс средней и пиковой ' +
        "задержки, памяти и пропускной способности для заданного профиля.</p></div>" +
        '<div class="tree"><span>Расчётный объём дерева</span><b>' +
        integer.format(data.estimated_nodes) +
        ' узлов</b></div><div class="metrics">' +
        metricRow(
            "Средняя задержка",
            number.format(best.mean_latency) + " мкс",
            quality("mean_latency", best.mean_latency),
        ) +
        metricRow(
            "Пиковая задержка p99",
            number.format(best.p99_latency) + " мкс",
            quality("p99_latency", best.p99_latency),
        ) +
        metricRow("Память структуры", memory, quality("memory", best.memory)) +
        metricRow(
            "Пропускная способность",
            integer.format(best.throughput) + " оп/с",
            quality("throughput", best.throughput, false),
        ) +
        '</div><p class="compare-title">Сравнение: задержка · память · скорость</p><div class="approaches">' +
        approaches +
        "</div></div>";
}

form.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!form.reportValidity() || !updateProbabilities()) return;
    button.disabled = true;
    button.querySelector("span").textContent = "Считаем метрики…";
    try {
        const response = await fetch("/api/recommend", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload()),
        });
        const data = await response.json();
        if (!response.ok) {
            throw new Error(
                data.errors?.probabilities ||
                    data.error ||
                    "Проверьте параметры",
            );
        }
        render(data);
        if (window.innerWidth < 980)
            results.scrollIntoView({ behavior: "smooth" });
    } catch (error) {
        results.innerHTML =
            '<div class="server-error"><p class="eyebrow">Не удалось выполнить расчёт</p>' +
            "<h2>Что-то пошло не так</h2><p>" +
            error.message +
            "</p></div>";
    } finally {
        button.disabled = false;
        button.querySelector("span").textContent = "Подобрать файловую систему";
    }
});

updateProbabilities();
