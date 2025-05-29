import sys
import pandas as pd

def load_data(filename: str) -> pd.DataFrame:
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

def print_stats(df: pd.DataFrame):
    config = 0
    prev_time = 0.0

    print(f"{'config':<15}{'bytes':<15}{'time(s)':<15}{'bw(MB/s)':<15}{'cycles':<15}")

    for row in df.itertuples():
        stat_name = row.StatisticName.strip()
        stat_value = row._asdict()['_7']

        if stat_name == 'total_bytes_write':
            bytes = stat_value

        elif stat_name == 'cycles':
            curr_time = row.SimTime / 1e+12

            time = curr_time - prev_time
            bw = ((bytes / 1.0e+06) / time) if (time > 0) else 0.0
            cycles = stat_value

            print(f"{config:<15}{bytes:<15}{time:<15g}{bw:<15.2f}{cycles:<15}")

            config += 1
            prev_time = curr_time

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f'Usage: python3 {sys.argv[0]} <filename>')
        sys.exit(1)

    filename = sys.argv[1]
    df = load_data(filename)
    print_stats(df)
