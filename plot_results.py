import pandas as pd
import matplotlib.pyplot as plt
import os

output_dir = 'results'
csv_filename = os.path.join(output_dir, 'results.csv')

if not os.path.exists(csv_filename):
    print(f"Помилка: Файл {csv_filename} не знайдено")
    exit()

try:
    df = pd.read_csv(csv_filename)
except Exception as e:
    exit()

sizes = df['Elements'].unique()

for size in sizes:
    df_size = df[df['Elements'] == size]

    # Витягуємо час послідовного алгоритму
    seq_time = df_size[df_size['Method'] == 'Sequential']['Time_ms'].iloc[0]

    mutex_data = df_size[df_size['Method'] == 'Mutex']
    cas_data = df_size[df_size['Method'] == 'CAS']

    plt.figure(figsize=(10, 6))

    plt.plot(mutex_data['Threads'].astype(str), mutex_data['Time_ms'],
             marker='o', linewidth=2, color='red', label='Блокуючий (Mutex)')

    plt.plot(cas_data['Threads'].astype(str), cas_data['Time_ms'],
             marker='s', linewidth=2, color='green', label='Неблокуючий (CAS)')

    plt.axhline(y=seq_time, color='blue', linestyle='--', linewidth=2,
                label=f'Послідовний ({seq_time:.2f} мс)')

    plt.title(f'Залежність часу від кількості потоків\nРозмір масиву: {size:,} елементів', fontsize=14)
    plt.xlabel('Кількість потоків', fontsize=12)
    plt.ylabel('Час виконання (мс)', fontsize=12)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(fontsize=11)

    plt.tight_layout()

    filename = f'plot_elements_{size}.png'
    filepath = os.path.join(output_dir, filename)

    plt.savefig(filepath, dpi=300)
    print(f"графік: {filepath}")

    plt.close()

print(f"\nГотово '{output_dir}'.")