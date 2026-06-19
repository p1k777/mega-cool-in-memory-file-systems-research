import sys

def predict_metrics(params: dict) -> dict:
    D = params['D']
    W = params['W']
    F = params['F']
    P_read = params['P_read']
    P_ls = params['P_ls']
    P_mv = params['P_mv']
    P_find = params['P_find']
    dist = params['Dist']
    alpha = params['Zipf_Alpha']
    
    estimated_nodes = int((W * D) * F * 100)
    if estimated_nodes < 500: 
        estimated_nodes = 500

    zipf_bonus = 1.0
    if dist == 'zipf' and alpha is not None:
        zipf_bonus = max(0.4, 1.0 - (alpha * 0.25))

    latency_A = (4.0 + (D * 1.8) + (P_ls * 45.0) + (P_find * 110.0)) * zipf_bonus
    memory_A = estimated_nodes * 48  
    throughput_A = 1000000.0 / latency_A

    latency_B = (1.8 + (P_mv * D * 180.0) + (P_find * 30.0)) * (zipf_bonus * 0.8 if dist == 'zipf' else 1.0)
    memory_B = estimated_nodes * 160  
    throughput_B = 1000000.0 / latency_B

    latency_C = 2.5 + (P_ls * estimated_nodes * 0.02) + (P_find * estimated_nodes * 0.05)
    memory_C = estimated_nodes * 112  
    throughput_C = 1000000.0 / latency_C

    return {
        'A': {'mean_latency': latency_A, 'p99_latency': latency_A * 1.9, 'memory': memory_A, 'throughput': throughput_A},
        'B': {'mean_latency': latency_B, 'p99_latency': latency_B * 1.25, 'memory': memory_B, 'throughput': throughput_B},
        'C': {'mean_latency': latency_C, 'p99_latency': latency_C * 1.4, 'memory': memory_C, 'throughput': throughput_C}
    }

def get_input_scalar(prompt: str, min_val, max_val, is_int=False):
    while True:
        try:
            val_str = input(prompt).strip()
            val = int(val_str) if is_int else float(val_str)
            if min_val <= val <= max_val:
                return val
            print(f"Ошибка: Значение должно быть в диапазоне от {min_val} до {max_val}.")
        except ValueError:
            print("Ошибка: Введите корректное число")

def get_input_distribution():
    while True:
        dist_type = input("Тип распределения доступов (uniform / zipf): ").strip().lower()
        if dist_type == 'uniform':
            return dist_type, None
        if dist_type == 'zipf':
            alpha = get_input_scalar("Введите коэффициент alpha (дробь от 0.1 до 3.0): ", 0.1, 3.0)
            return dist_type, alpha
        print("Ошибка: Допустимы только 'uniform' или 'zipf'")

def collect_and_validate_params() -> dict:
    print("Параметры виртуальной ФС:")
    D = get_input_scalar("Глубина дерева D (целое от 1 до 50): ", 1, 50, is_int=True)
    W = get_input_scalar("Ширина дерева W (целое от 1 до 100): ", 1, 5000, is_int=True)
    F = get_input_scalar("Коэффициент заполнения F (дробь от 0.01 до 1.0): ", 0.01, 1.0)

    print("Профиль нагрузки:")
    print("Сумма всех 6 вероятностей должна быть равна 1.0")
    
    while True:
        p_read = get_input_scalar("  Вероятность чтения P_read: ", 0.0, 1.0)
        p_write = get_input_scalar("  Вероятность записи P_write: ", 0.0, 1.0)
        p_mkdir = get_input_scalar("  Вероятность создания папок P_mkdir: ", 0.0, 1.0)
        p_ls = get_input_scalar("  Вероятность вывода списка P_ls: ", 0.0, 1.0)
        p_mv = get_input_scalar("  Вероятность перемещения P_mv: ", 0.0, 1.0)
        p_find = get_input_scalar("  Вероятность глобального поиска P_find: ", 0.0, 1.0)
        
        total_p = p_read + p_write + p_mkdir + p_ls + p_mv + p_find
        if abs(total_p - 1.0) < 1e-4:
            break
        print(f"Ошибка валидации: Сумма вероятностей равна {total_p:.4f} вместо 1.0")
        print("Повторите ввод блока вероятностей заново")

    print("Законы распределения потока:")
    dist_type, alpha = get_input_distribution()
    loc = get_input_scalar("Коэффициент локальности Loc (дробь от 0.0 до 1.0): ", 0.0, 1.0)

    return {
        'D': D, 'W': W, 'F': F,
        'P_read': p_read, 'P_write': p_write, 'P_mkdir': p_mkdir,
        'P_ls': p_ls, 'P_mv': p_mv, 'P_find': p_find,
        'Dist': dist_type, 'Zipf_Alpha': alpha, 'Loc': loc
    }

def select_best_approach_auto(predictions: dict) -> tuple:
    w = [0.35, 0.20, 0.20, 0.25]
    candidates = list(predictions.keys())

    all_lat = [predictions[c]['mean_latency'] for c in candidates]
    all_p99 = [predictions[c]['p99_latency'] for c in candidates]
    all_mem = [predictions[c]['memory'] for c in candidates]
    all_thr = [predictions[c]['throughput'] for c in candidates]

    max_lat, min_lat = max(all_lat), min(all_lat)
    max_p99, min_p99 = max(all_p99), min(all_p99)
    max_mem, min_mem = max(all_mem), min(all_mem)
    max_thr, min_thr = max(all_thr), min(all_thr)

    def normalize(val, min_v, max_v, is_cost=True):
        if max_v == min_v: 
            return 1.0
        return (max_v - val) / (max_v - min_v) if is_cost else (val - min_v) / (max_v - min_v)

    scores = {}
    for cand in candidates:
        m = predictions[cand]
        
        n_lat = max(normalize(m['mean_latency'], min_lat, max_lat, is_cost=True), 0.01)
        n_p99 = max(normalize(m['p99_latency'], min_p99, max_p99, is_cost=True), 0.01)
        n_mem = max(normalize(m['memory'], min_mem, max_mem, is_cost=True), 0.01)
        n_thr = max(normalize(m['throughput'], min_thr, max_thr, is_cost=False), 0.01)

        scores[cand] = (n_lat ** w[0]) * (n_p99 ** w[1]) * (n_mem ** w[2]) * (n_thr ** w[3])

    best_cand = max(scores, key=lambda k: scores[k])
    return best_cand, scores

def main():
    print("СИСТЕМА АВТОМАТИЧЕСКОГО ПОДБОРА IN-MEMORY ФАЙЛОВЫХ СИСТЕМ")
    print()
    
    params = collect_and_validate_params()
    predictions = predict_metrics(params)
    best, scores = select_best_approach_auto(predictions)
    
    print()
    print("-" * 66)
    
    for app, metrics in predictions.items():
        status_prefix = "[РЕКОМЕНДУЕТСЯ]" if app == best else "               "
        print(f"{status_prefix} Подход {app}:")
        print(f"    Средняя задержка:       {metrics['mean_latency']:.2f} мкс")
        print(f"    Пиковая задержка (p99):  {metrics['p99_latency']:.2f} мкс")
        print(f"    Объем памяти структуры: {metrics['memory']:,} байт")
        print(f"    Пропускная способность: {metrics['throughput']:.2f} оп/сек")
        print(f"    Коэффициент пригодности: {scores[app]:.4f}")
        print("-" * 66)
        
    print(f"ИТОГОВОЕ РЕШЕНИЕ: Для заданных условий оптимально развернуть подход {best}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nПрограмма завершена")
        sys.exit(0)