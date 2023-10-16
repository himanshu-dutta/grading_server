import pandas as pd
df=pd.read_csv('server_stats.csv')
df=df.groupby('num_clients').avg()
df.to_csv('server_stats_output.csv', index=True)