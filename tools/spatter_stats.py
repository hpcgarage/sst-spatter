import sys
import pandas as pd
import json

def load_stats(filename: str) -> pd.DataFrame:
    df = pd.DataFrame()

    try:
        df = pd.read_csv(filename)
        df.columns = df.columns.str.strip()
    except Exception as error:
        print(error)
        sys.exit(1)
    else:
        df.columns = df.columns.str.strip()

    return df

def get_kernel_data(filename: str) -> list[str]:
    try:
        with open(filename, 'r') as file:
            patterns = json.load(file)
    except Exception as error:
        print(error)
        sys.exit(1)

    kernel_data = []
    for pattern in patterns:
        kernel = pattern.get('kernel', 'gather')
        kernel_data.append(kernel.lower())

    return kernel_data

def print_stats(stats_data: pd.DataFrame, kernel_data: list[str]):
    config = 0
    prev_time = 0.0

    print(f"{'config':<15}{'bytes':<15}{'time(s)':<15}{'bw(MB/s)':<15}{'cycles':<15}")

    for row in stats_data.itertuples():
        stat_name = row.StatisticName.strip()
        stat_value = row._asdict()['_7']

        if stat_name == 'total_bytes_write':
            byteCount = (stat_value * 2) if kernel_data[config] == 'gs' else stat_value

        elif stat_name == 'cycles':
            cycles = stat_value

        elif stat_name == 'configTime':
            curr_time = stat_value / 1e+12

            time = curr_time - prev_time
            bw = ((byteCount / 1.0e+06) / time) if (time > 0) else 0.0

            print(f"{config:<15}{byteCount:<15}{time:<15g}{bw:<15.2f}{cycles:<15}")

            config += 1
            prev_time = curr_time

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'Usage: python3 {sys.argv[0]} <stats.csv> <trace.json>')
        sys.exit(1)

    stats_file = sys.argv[1]
    trace_file = sys.argv[2]

    stats_data = load_stats(stats_file)
    kernel_data = get_kernel_data(trace_file)

    print_stats(stats_data, kernel_data)
