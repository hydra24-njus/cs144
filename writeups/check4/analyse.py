import re
import matplotlib.pyplot as plt
from datetime import datetime, timezone
import numpy as np

def parse_log(log_file):
        # 用于匹配日志中icmp_seq的正则表达式
    seq_pattern = re.compile(r'icmp_seq=(\d+)')

    # 用于保存日志中所有出现的icmp_seq
    seen_seqs = set()

    # 遍历日志文件并提取icmp_seq
    last_seq = None
    with open(log_file, 'r') as file:
        for line in file:
            match = seq_pattern.search(line)
            if match:
                seq = int(match.group(1))
                seen_seqs.add(seq)

    # 找出缺失的icmp_seq
    min_seq = min(seen_seqs) if seen_seqs else 0
    max_seq = max(seen_seqs) if seen_seqs else 0
    missing_seqs = [seq for seq in range(min_seq, max_seq + 1) if seq not in seen_seqs]
    return seen_seqs,missing_seqs

def find_longest_success_streak(seen_seqs):
    # 默认的 total_requests 使用 seen_seqs 中的最大值
    total_requests = max(seen_seqs) if seen_seqs else 0

    # 计算最长连续ping成功的区间
    longest_success_streak = 0
    current_success_streak = 0
    success_start = None
    longest_success_range = (None, None)

    # 遍历seen_seqs，计算连续成功区间
    for seq in range(1, total_requests + 1):
        if seq in seen_seqs:
            # 连续成功
            if current_success_streak == 0:
                success_start = seq
            current_success_streak += 1
        else:
            # 如果当前成功区间长度大于已记录的最长成功区间长度，更新
            if current_success_streak > longest_success_streak:
                longest_success_streak = current_success_streak
                longest_success_range = (success_start, seq - 1)
            # 重置当前成功区间长度
            current_success_streak = 0

    # 处理最后的成功区间
    if current_success_streak > longest_success_streak:
        longest_success_streak = current_success_streak
        longest_success_range = (success_start, total_requests)

    return longest_success_streak, longest_success_range

def find_longest_missing_streak(missing_seqs):

    # 计算最长连续ping失败的区间
    longest_failure_streak = 0
    current_failure_streak = 0
    failure_start = None
    longest_failure_range = (None, None)

    if missing_seqs:
        # 遍历缺失的icmp_seq，计算最长的ping失败区间
        for i in range(1, len(missing_seqs)):
            if missing_seqs[i] == missing_seqs[i - 1] + 1:
                if current_failure_streak == 0:
                    failure_start = missing_seqs[i - 1]
                    current_failure_streak = 1
                current_failure_streak += 1
            else:
                # 更新最长ping失败区间
                if current_failure_streak > longest_failure_streak:
                    longest_failure_streak = current_failure_streak
                    longest_failure_range = (failure_start, missing_seqs[i - 1])

                # 重置失败区间
                current_failure_streak = 0

        # 处理最后一段连续失败区间
        if current_failure_streak > longest_failure_streak:
            longest_failure_streak = current_failure_streak
            longest_failure_range = (failure_start, missing_seqs[-1])

    return longest_failure_streak, longest_failure_range

def calculate_packet_loss_correlation(seen_seqs, missing_seqs):
    # 总请求数是 seen_seqs 和 missing_seqs 中的最大序列号
    total_requests = max(max(seen_seqs), max(missing_seqs)) if seen_seqs else 0
    
    # 计算总的无条件包交付率
    total_packets = total_requests  # 假设请求序列号是从1开始到最大值
    successful_replies = len(seen_seqs)
    unconditional_reply_rate = successful_replies / total_packets if total_packets > 0 else 0

    # 计算条件概率：给定N请求成功，N+1也成功的概率
    success_to_success = 0
    for seq in range(1, total_requests):
        if seq in seen_seqs and (seq + 1) in seen_seqs:
            success_to_success += 1
    p_success_given_success = success_to_success / successful_replies if successful_replies > 0 else 0

    # 计算条件概率：给定N请求失败，N+1成功的概率
    failure_to_success = 0
    for seq in range(1, total_requests):
        if seq not in seen_seqs and (seq + 1) in seen_seqs:
            failure_to_success += 1
    p_success_given_failure = failure_to_success / len(missing_seqs) if len(missing_seqs) > 0 else 0

    return unconditional_reply_rate, p_success_given_success, p_success_given_failure

def parse_rtt(log_file):
    # 用于匹配日志中的时间戳和rtt值
    log_pattern = re.compile(r'\[([\d.]+)\].*time=([\d.]+) ms')

    # 用于保存日志中提取的时间和rtt
    time_rtt_pairs = []

    with open(log_file, 'r') as file:
        for line in file:
            match = log_pattern.search(line)
            if match:
                # 提取时间戳和rtt值
                timestamp = float(match.group(1))  # 时间戳
                rtt = float(match.group(2))  # rtt值，单位为毫秒
                time_rtt_pairs.append((timestamp, rtt))

    return time_rtt_pairs

def calculate_max_min_rtt(time_rtt_data):
    if time_rtt_data:
        # 提取所有rtt值
        rtt_values = [rtt for _, rtt in time_rtt_data]
        max_rtt = max(rtt_values)
        min_rtt = min(rtt_values)
    else:
        max_rtt = min_rtt = None
    
    return max_rtt, min_rtt

def plot_rtt_vs_time(time_rtt_data):
    # 将时间戳转换为实际时间，使用时区感知的UTC时间
    times = [datetime.fromtimestamp(timestamp, timezone.utc) for timestamp, _ in time_rtt_data]
    rtts = [rtt for _, rtt in time_rtt_data]

    # 创建图表
    plt.figure(figsize=(30, 6), dpi=400)
    plt.plot(times, rtts,linestyle='-', color='b', linewidth=1)

    # 设置标题和标签
    plt.title('RTT vs Time')
    plt.xlabel('Time of Day')
    plt.ylabel('RTT (ms)')

    # 格式化x轴为日期时间
    plt.gca().xaxis.set_major_formatter(plt.matplotlib.dates.DateFormatter('%H:%M:%S'))

    # 自动旋转x轴标签以避免重叠
    plt.xticks(rotation=45)

    # 显示图表
    plt.tight_layout()
    plt.savefig('time.png')

def plot_rtt_histogram(time_rtt_data):
    # 提取所有 RTT 值
    rtts = [rtt for _, rtt in time_rtt_data]

    # 创建直方图
    plt.figure(figsize=(10, 6), dpi=200)
    plt.hist(rtts, bins=30, color='b', edgecolor='black', alpha=0.7)

    # 设置标题和标签
    plt.title('RTT Distribution (Histogram)')
    plt.xlabel('RTT (ms)')
    plt.ylabel('Frequency')

    # 显示图表
    plt.tight_layout()
    plt.savefig('histogram.png')

def plot_rtt_cdf(time_rtt_data):
    # 提取所有 RTT 值
    rtts = [rtt for _, rtt in time_rtt_data]

    # 排序 RTT 值
    rtts_sorted = np.sort(rtts)

    # 计算 CDF
    cdf = np.arange(1, len(rtts_sorted) + 1) / len(rtts_sorted)

    # 创建 CDF 图
    plt.figure(figsize=(10, 6), dpi=200)
    plt.plot(rtts_sorted, cdf, marker='.', linestyle='-', color='b', markersize=5)

    # 设置标题和标签
    plt.title('RTT Cumulative Distribution Function (CDF)')
    plt.xlabel('RTT (ms)')
    plt.ylabel('CDF')

    # 显示图表
    plt.tight_layout()
    plt.savefig('cdf.png')

def plot_rtt_correlation(time_rtt_data):
    # 提取 RTT 值
    rtts = [rtt for _, rtt in time_rtt_data]

    # 创建一对连续的 RTT：RTT(N) 和 RTT(N+1)
    rtt_pairs = [(rtts[i], rtts[i + 1]) for i in range(len(rtts) - 1)]

    # 将连续的 RTT 值分成两个列表
    rtt_n = [pair[0] for pair in rtt_pairs]
    rtt_n_plus_1 = [pair[1] for pair in rtt_pairs]

    # 创建散点图
    plt.figure(figsize=(10, 6), dpi=200)
    plt.scatter(rtt_n, rtt_n_plus_1, color='b', alpha=0.5)

    # 设置标题和标签
    plt.title('Correlation between RTT of ping #N and RTT of ping #N+1')
    plt.xlabel('RTT of ping #N (ms)')
    plt.ylabel('RTT of ping #N+1 (ms)')

    # 显示图表
    plt.tight_layout()
    plt.savefig('correlation.png')


log_file = 'data.txt'  # 修改为你的日志文件路径
seen_seqs,missing_seqs=parse_log(log_file)
longest_success_streak, longest_success_range = find_longest_success_streak(seen_seqs)
longest_failure_streak, longest_failure_range = find_longest_missing_streak(missing_seqs)

if missing_seqs:
    print(f"缺少的icmp_seq序列号: {', '.join(map(str, missing_seqs))}")
else:
    print("没有缺少的icmp_seq序列号")

print(f"最长连续成功ping的数量: {longest_success_streak}，区间: {longest_success_range}")
print(f"最长ping失败的区间长度: {longest_failure_streak}，区间: {longest_failure_range}")

unconditional_reply_rate, p_success_given_success, p_success_given_failure = calculate_packet_loss_correlation(seen_seqs, missing_seqs)

print(f"总体包成功率: {unconditional_reply_rate:.4f}")
print(f"给定N请求成功，N+1也成功的条件概率: {p_success_given_success:.4f}")
print(f"给定N请求失败，N+1成功的条件概率: {p_success_given_failure:.4f}")

time_rtt_data = parse_rtt(log_file)
max_rtt, min_rtt = calculate_max_min_rtt(time_rtt_data)

# 输出最大和最小 RTT
if max_rtt is not None and min_rtt is not None:
    print(f"最大 RTT: {max_rtt}ms")
    print(f"最小 RTT: {min_rtt}ms")
else:
    print("日志文件为空或没有有效的 RTT 数据")

plot_rtt_vs_time(time_rtt_data)
plot_rtt_histogram(time_rtt_data)
plot_rtt_cdf(time_rtt_data)
plot_rtt_correlation(time_rtt_data)