import sys
import json

import pandas as pd

DEFAULT_KERNEL = 'gather'

def load_stats(filename: str) -> pd.DataFrame:
    try:
        df = pd.read_csv(filename)
        df.columns = df.columns.str.strip().str.replace('.', '_')
        return df
    except Exception as error:
        print(f'Error: {error}.')
        sys.exit(1)

def extract_kernels(filename: str) -> list[str]:
    try:
        with open(filename, 'r') as file:
            patterns = json.load(file)
    except Exception as error:
        print(f'Error: {error}.')
        sys.exit(1)

    return [pattern.get('kernel', DEFAULT_KERNEL).lower() for pattern in patterns]

def print_stats(stats_df: pd.DataFrame, kernel_list: list[str]):
    config_index = 0
    byte_count = 0
    cycles = 0

    print(f"{'config':<15}{'bytes':<15}{'time(s)':<15}{'bw(MB/s)':<15}{'cycles':<15}")

    for row in stats_df.itertuples():
        stat_name = row.StatisticName.strip()
        stat_value = row.Sum_u64

        if stat_name == 'total_bytes_write':
            byte_count = (stat_value * 2) if kernel_list[config_index] == 'gs' else stat_value

        elif stat_name == 'cycles':
            cycles = stat_value

        elif stat_name == 'config_time':
            time = stat_value / 1e+12
            bandwidth = ((byte_count / 1.0e+06) / time) if (time > 0) else 0.0

            print(f'{config_index:<15}{byte_count:<15}{time:<15g}{bandwidth:<15.2f}{cycles:<15}')
            config_index += 1

def main():
    if len(sys.argv) != 3:
        print(f'Usage: python3 {sys.argv[0]} <sst-stats.csv> <spatter-pattern.json>')
        sys.exit(1)

    stats_file = sys.argv[1]
    pattern_file = sys.argv[2]

    stats_df = load_stats(stats_file)
    kernel_list = extract_kernels(pattern_file)

    print_stats(stats_df, kernel_list)

if __name__ == '__main__':
    main()
