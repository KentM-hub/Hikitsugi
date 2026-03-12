import os
import pandas as pd

PAGE_SHIFT = 12 
DEBUG_FILE = 'DEBUG_OUTPUT_DETACHED.txt'
RANDOM_STRIDE_FILE = 'random_numbers_stride.txt'
MAP_SIZES_TO_TEST = [4, 8, 10, 12, 16, 32, 64, 128]
HISTORY_MAP_SIZE = 3 

def load_random_strides(filepath):
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found.")
        return []
    strides = []
    with open(filepath, 'r') as f:
        content = f.read().replace(',', ' ')
        parts = content.split()
        for part in parts:
            try:
                strides.append(int(part)) 
            except ValueError:
                continue
    return strides

def write_debug_log(log_file, step, i, pc, current_ppage, pc_entry, message, stride_pattern=None, status=None, found_in_file=None):
    log_file.write(f"[{step} | Trace Index: {i}]\n")
    log_file.write(f"  PC: {pc}\n")
    log_file.write(f"  Current PPage: {hex(current_ppage) if current_ppage is not None else 'None'}\n\n")
    
    target = pc_entry['target']
    history = pc_entry['history']
    
    log_file.write(f"  >>> ACTIVE PREFETCH TARGET <<<\n")
    log_file.write(f"    Target PPage : {hex(target['ppage']) if target['ppage'] is not None else 'None'}\n")
    log_file.write(f"    Access Count : {target['count']}\n")
    log_file.write(f"    Entry Stride : {target['stride']}\n\n")
    
    log_file.write(f"  >>> HISTORY MAP <<<\n")
    for idx, h in enumerate(history):
        if h:
            log_file.write(f"    P{idx+1}: {hex(h['ppage'])} | Count: {h['count']} | Stride: {h['stride']}\n")
    
    if stride_pattern:
        log_file.write(f"\n  --- Pattern Matching Analysis ---\n")
        log_file.write(f"    Current Pattern (s1, s2, s3) : {stride_pattern}\n")
        log_file.write(f"    Status                       : {status}\n")

    log_file.write(f"\n  Message: {message}\n")
    log_file.write("-" * 60 + "\n")

def simulate_prefetch(trace_data, max_map_size, random_strides):
    prefetch_map = {} 
    total_accesses = 0
    prefetch_attempts = 0
    prefetch_hits = 0
    page_boundary_crossings = 0

    debug_log = open(DEBUG_FILE, 'w')
    debug_log.write(f"*** Prefetch Simulator (Map Size: {max_map_size}) ***\n")

    for i, (pc_str, ppage_str) in enumerate(trace_data):
        try:
            current_ppage = int(ppage_str, 16) >> PAGE_SHIFT
        except ValueError:
            continue
            
        total_accesses += 1

        if pc_str in prefetch_map:
            pc_entry = prefetch_map[pc_str]
            pc_entry['last_access'] = i 
            
            if pc_entry['target']['ppage'] == current_ppage:
                old_count = pc_entry['target']['count']
                pc_entry['target']['count'] += 1
                
                valid_histories = [h for h in pc_entry['history'] if h is not None]
                if len(valid_histories) >= 2:
                    threshold = min(valid_histories[0]['count'], valid_histories[1]['count'])
                    if threshold > 0 and old_count == threshold:
                        if len(valid_histories) >= 3:
                            s1, s2, s3 = valid_histories[1]['stride'], valid_histories[0]['stride'], pc_entry['target']['stride']
                            target_pattern = (s1, s2, s3)
                            
                            found_base_pattern = False
                            expected_s3 = None
                            
                            for j in range(len(random_strides) - 2):
                                if random_strides[j] == s1 and random_strides[j+1] == s2:
                                    found_base_pattern = True
                                    prefetch_attempts += 1 
                                    expected_s3 = random_strides[j+2]

                                    if expected_s3 == s3:
                                        prefetch_hits += 1
                                        status = "HIT"
                                        msg = f"SUCCESS: Pattern {target_pattern} exists"
                                    else:
                                        status = "MISS"
                                        msg = f"FAILED: Pattern ({s1}, {s2}, {expected_s3}) exists"
                                    
                                    write_debug_log(debug_log, "PREFETCH_ATTEMPT", i, pc_str, current_ppage, pc_entry, 
                                                   msg, stride_pattern=target_pattern, status=status)
                                    break 
                
            else:
                page_boundary_crossings += 1
                old_target = pc_entry['target'].copy()
                new_stride = current_ppage - old_target['ppage']

                found_idx = -1
                for idx, h in enumerate(pc_entry['history']):
                    if h and h['ppage'] == current_ppage:
                        found_idx = idx
                        break
                
                if found_idx != -1:
                    matched_entry = pc_entry['history'].pop(found_idx)
                    matched_entry['count'] += 1
                    matched_entry['stride'] = new_stride
                    pc_entry['history'].insert(0, old_target)
                    pc_entry['target'] = matched_entry
                else:
                    pc_entry['history'].insert(0, old_target)
                    pc_entry['target'] = {'ppage': current_ppage, 'count': 1, 'stride': new_stride}
                
                if len(pc_entry['history']) > HISTORY_MAP_SIZE:
                    pc_entry['history'] = pc_entry['history'][:HISTORY_MAP_SIZE]
                
                write_debug_log(debug_log, "PAGE_CHANGE", i, pc_str, current_ppage, pc_entry, 
                               f"MOVED: {hex(old_target['ppage'])} -> {hex(current_ppage)}. STRIDE={new_stride}")

        else:
            if len(prefetch_map) >= max_map_size:
                oldest_pc = min(prefetch_map, key=lambda k: prefetch_map[k]['last_access'])
                del prefetch_map[oldest_pc]
            
            prefetch_map[pc_str] = {
                'target': {'ppage': current_ppage, 'count': 1, 'stride': 0},
                'history': [None] * HISTORY_MAP_SIZE,
                'last_access': i 
            }

    debug_log.close()
    return total_accesses, prefetch_attempts, prefetch_hits, page_boundary_crossings

def run_experiment_and_report(input_file_path, map_sizes, output_csv, random_stride_file):
    random_strides = load_random_strides(random_stride_file)
    if not os.path.exists(input_file_path): return

    trace_data = []
    with open(input_file_path, 'r') as f:
        for line in f:
            if "---" in line or not line.strip(): continue
            parts = line.strip().split()
            if len(parts) >= 3: trace_data.append((parts[0], parts[2]))

    results = []
    for size in map_sizes:
        total, attempts, hits, crossings = simulate_prefetch(trace_data, size, random_strides)
        accuracy = (hits / attempts * 100) if attempts > 0 else 0.0
        results.append({
            'Map Size': size, 
            'Attempts(Exe/Boundary)': f"{attempts}/{crossings}", 
            'Hits': hits, 
            'Accuracy (%)': round(accuracy, 2)
        })
    print(pd.DataFrame(results).to_markdown(index=False))

if __name__ == '__main__':
    run_experiment_and_report('physical_page_start_trace.txt', MAP_SIZES_TO_TEST, 
                              'prefetch_accuracy_results_stride.csv', 'random_numbers_stride.txt')