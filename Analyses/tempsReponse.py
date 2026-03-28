import csv
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

# Read CSV file with comma-separated values
df = pd.read_csv('Analyses/temps de reponse premier prototype.csv')

# If first column contains comma-separated values, split them
if ',' in df.iloc[0, 0]:
    df[[f'col_{i}' for i in range(len(df.iloc[0, 0].split(',')))]] = df.iloc[:, 0].str.split(',', expand=True)
    df = df.drop(df.columns[0], axis=1)

print(df)

# Parse timestamp and plot tension_ch1 and tension_ch2
df['timestamp'] = pd.to_datetime(df['timestamp'], format='%Y-%m-%dT%H:%M:%S.%f')

plt.figure(figsize=(12, 6))
plt.plot(df['timestamp'], df['tension_ch1']/max(df['tension_ch1']), label='tension_ch1')
plt.plot(df['timestamp'], df['tension_ch2']/max(df['tension_ch2']), label='tension_ch2')
plt.xlabel('Timestamp')
plt.ylabel('Tension')
plt.legend()
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()

