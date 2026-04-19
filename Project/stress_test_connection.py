#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LChat 服务器并发连接数压力测试工具
测试服务器的最大并发连接上限
"""

import asyncio
import time
import statistics
import sys
from dataclasses import dataclass, field
from typing import List, Dict
from datetime import datetime


print("="*70)
print("LChat 服务器并发连接数压力测试工具")
print("="*70)
print(f"Python 版本: {sys.version}")
print(f"开始时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
print("="*70)
print()


@dataclass
class ConnectionResult:
    """单个连接的测试结果"""
    client_id: int
    connected: bool = False
    connect_time: float = 0.0  # 连接建立时间(毫秒)
    error: str = ""


@dataclass
class StressTestResult:
    """压力测试总体结果"""
    total_attempts: int = 0
    successful_connections: int = 0
    failed_connections: int = 0
    connection_times: List[float] = field(default_factory=list)
    peak_connections: int = 0
    test_duration: float = 0.0
    errors: Dict[str, int] = field(default_factory=dict)
    
    @property
    def success_rate(self) -> float:
        if self.total_attempts == 0:
            return 0.0
        return (self.successful_connections / self.total_attempts) * 100
    
    @property
    def avg_connection_time(self) -> float:
        if not self.connection_times:
            return 0.0
        return statistics.mean(self.connection_times)
    
    @property
    def median_connection_time(self) -> float:
        if not self.connection_times:
            return 0.0
        return statistics.median(self.connection_times)
    
    @property
    def p95_connection_time(self) -> float:
        if not self.connection_times:
            return 0.0
        sorted_times = sorted(self.connection_times)
        index = int(len(sorted_times) * 0.95)
        return sorted_times[min(index, len(sorted_times) - 1)]
    
    @property
    def p99_connection_time(self) -> float:
        if not self.connection_times:
            return 0.0
        sorted_times = sorted(self.connection_times)
        index = int(len(sorted_times) * 0.99)
        return sorted_times[min(index, len(sorted_times) - 1)]


class ConnectionStressTester:
    """并发连接数压力测试器"""
    
    def __init__(self, host: str = '127.0.0.1', port: int = 8090):
        self.host = host
        self.port = port
        self.result = StressTestResult()
        self.active_connections = 0
        self.max_concurrent = 0
        self._lock = asyncio.Lock()
        
    async def try_connect(self, client_id: int) -> ConnectionResult:
        """尝试建立一个TCP连接"""
        result = ConnectionResult(client_id=client_id)
        
        try:
            start_time = time.time()
            
            # 尝试建立连接
            reader, writer = await asyncio.wait_for(
                asyncio.open_connection(self.host, self.port),
                timeout=10.0  # 10秒超时
            )
            
            connect_time = (time.time() - start_time) * 1000  # 转换为毫秒
            
            result.connected = True
            result.connect_time = connect_time
            
            # 更新活跃连接数
            async with self._lock:
                self.active_connections += 1
                if self.active_connections > self.max_concurrent:
                    self.max_concurrent = self.active_connections
            
            # 保持连接一段时间(模拟真实场景)
            await asyncio.sleep(5.0)
            
            # 关闭连接
            writer.close()
            await writer.wait_closed()
            
            async with self._lock:
                self.active_connections -= 1
                
        except asyncio.TimeoutError:
            result.error = "Connection timeout"
            self.result.errors["timeout"] = self.result.errors.get("timeout", 0) + 1
            
        except ConnectionRefusedError:
            result.error = "Connection refused"
            self.result.errors["refused"] = self.result.errors.get("refused", 0) + 1
            
        except OSError as e:
            result.error = f"OS error: {str(e)}"
            self.result.errors["os_error"] = self.result.errors.get("os_error", 0) + 1
            
        except Exception as e:
            result.error = f"Unknown error: {str(e)}"
            self.result.errors["unknown"] = self.result.errors.get("unknown", 0) + 1
        
        return result
    
    async def test_batch(self, batch_size: int, start_id: int) -> List[ConnectionResult]:
        """测试一批并发连接"""
        tasks = [
            self.try_connect(start_id + i)
            for i in range(batch_size)
        ]
        
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        # 处理异常结果
        final_results = []
        for i, res in enumerate(results):
            if isinstance(res, Exception):
                conn_result = ConnectionResult(client_id=start_id + i)
                conn_result.error = f"Task exception: {str(res)}"
                final_results.append(conn_result)
            else:
                final_results.append(res)
        
        return final_results
    
    async def run_incremental_test(self, 
                                   start_users: int = 10,
                                   max_users: int = 1000,
                                   step: int = 50,
                                   delay_between_steps: float = 2.0):
        """
        递增式压力测试
        逐步增加并发用户数,找到服务器连接上限
        """
        print("="*70)
        print("LChat 服务器并发连接数压力测试")
        print("="*70)
        print(f"目标服务器: {self.host}:{self.port}")
        print(f"测试范围: {start_users} - {max_users} 并发连接")
        print(f"步进: {step}")
        print("="*70)
        print()
        
        overall_result = StressTestResult()
        
        for concurrent_users in range(start_users, max_users + 1, step):
            print(f"\n{'='*70}")
            print(f"测试批次: {concurrent_users} 个并发连接")
            print(f"{'='*70}")
            
            batch_start_time = time.time()
            
            # 执行一批连接测试
            results = await self.test_batch(concurrent_users, overall_result.total_attempts)
            
            batch_duration = time.time() - batch_start_time
            
            # 统计本批次结果
            batch_success = sum(1 for r in results if r.connected)
            batch_failed = sum(1 for r in results if not r.connected)
            batch_times = [r.connect_time for r in results if r.connected]
            
            # 更新总体结果
            overall_result.total_attempts += len(results)
            overall_result.successful_connections += batch_success
            overall_result.failed_connections += batch_failed
            overall_result.connection_times.extend(batch_times)
            
            # 计算本批次成功率
            batch_success_rate = (batch_success / len(results) * 100) if results else 0
            
            print(f"✓ 成功: {batch_success} | ✗ 失败: {batch_failed}")
            print(f"成功率: {batch_success_rate:.2f}%")
            print(f"耗时: {batch_duration:.2f} 秒")
            
            if batch_times:
                print(f"平均连接时间: {statistics.mean(batch_times):.2f} ms")
                print(f"P95连接时间: {sorted(batch_times)[int(len(batch_times)*0.95)]:.2f} ms")
            
            # 记录峰值连接数
            if self.max_concurrent > overall_result.peak_connections:
                overall_result.peak_connections = self.max_concurrent
            
            print(f"当前活跃连接: {self.active_connections}")
            print(f"历史峰值连接: {self.max_concurrent}")
            
            # 如果成功率低于阈值,认为达到服务器上限
            if batch_success_rate < 90:
                print(f"\n⚠️  警告: 成功率降至 {batch_success_rate:.2f}%,可能已达到服务器上限!")
                
                # 再测试一次确认
                print(f"\n进行二次验证测试...")
                verify_results = await self.test_batch(concurrent_users, overall_result.total_attempts)
                verify_success = sum(1 for r in verify_results if r.connected)
                verify_rate = (verify_success / len(verify_results) * 100) if verify_results else 0
                
                print(f"二次验证成功率: {verify_rate:.2f}%")
                
                if verify_rate < 90:
                    print(f"\n🎯 估计服务器最大并发连接数: {concurrent_users - step}")
                    break
            
            # 等待一段时间后继续下一批
            if concurrent_users + step <= max_users:
                print(f"\n等待 {delay_between_steps} 秒后继续下一批测试...")
                await asyncio.sleep(delay_between_steps)
        
        # 输出最终报告
        overall_result.test_duration = time.time() - batch_start_time
        self.print_final_report(overall_result)
        
        return overall_result
    
    async def run_fixed_test(self, concurrent_users: int, duration: int = 30):
        """
        固定并发数稳定性测试
        在指定并发数下持续运行指定时间
        """
        print("="*70)
        print(f"LChat 服务器稳定性测试 - {concurrent_users} 并发连接")
        print("="*70)
        print(f"目标服务器: {self.host}:{self.port}")
        print(f"并发用户数: {concurrent_users}")
        print(f"测试时长: {duration} 秒")
        print("="*70)
        print()
        
        test_result = StressTestResult()
        end_time = time.time() + duration
        
        round_count = 0
        
        while time.time() < end_time:
            round_count += 1
            print(f"\n--- 第 {round_count} 轮测试 ---")
            
            results = await self.test_batch(concurrent_users, test_result.total_attempts)
            
            batch_success = sum(1 for r in results if r.connected)
            batch_failed = sum(1 for r in results if not r.connected)
            batch_times = [r.connect_time for r in results if r.connected]
            
            test_result.total_attempts += len(results)
            test_result.successful_connections += batch_success
            test_result.failed_connections += batch_failed
            test_result.connection_times.extend(batch_times)
            
            success_rate = (batch_success / len(results) * 100) if results else 0
            
            print(f"成功: {batch_success} | 失败: {batch_failed} | 成功率: {success_rate:.2f}%")
            
            if batch_times:
                print(f"平均延迟: {statistics.mean(batch_times):.2f} ms")
            
            # 短暂休息
            await asyncio.sleep(1)
        
        test_result.test_duration = duration
        self.print_final_report(test_result)
        
        return test_result
    
    def print_final_report(self, result: StressTestResult):
        """打印最终测试报告"""
        print("\n" + "="*70)
        print("📊 最终测试报告")
        print("="*70)
        print(f"测试时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"服务器地址: {self.host}:{self.port}")
        print(f"-"*70)
        print(f"总连接尝试: {result.total_attempts}")
        print(f"成功连接数: {result.successful_connections}")
        print(f"失败连接数: {result.failed_connections}")
        print(f"成功率: {result.success_rate:.2f}%")
        print(f"峰值并发连接: {result.peak_connections}")
        print(f"测试持续时间: {result.test_duration:.2f} 秒")
        print(f"-"*70)
        
        if result.connection_times:
            print(f"\n连接时间统计:")
            print(f"  平均值: {result.avg_connection_time:.2f} ms")
            print(f"  中位数: {result.median_connection_time:.2f} ms")
            print(f"  P95: {result.p95_connection_time:.2f} ms")
            print(f"  P99: {result.p99_connection_time:.2f} ms")
            print(f"  最小值: {min(result.connection_times):.2f} ms")
            print(f"  最大值: {max(result.connection_times):.2f} ms")
        
        if result.errors:
            print(f"\n错误统计:")
            for error_type, count in result.errors.items():
                print(f"  {error_type}: {count} 次")
        
        print("="*70)
        
        # 性能评估
        print(f"\n💡 性能评估:")
        if result.success_rate >= 99:
            print("  ✅ 优秀 - 服务器表现稳定")
        elif result.success_rate >= 95:
            print("  ⚠️  良好 - 服务器基本稳定,可能有轻微波动")
        elif result.success_rate >= 90:
            print("  ⚠️  一般 - 建议优化服务器配置")
        else:
            print("  ❌ 较差 - 服务器可能存在瓶颈或配置问题")
        
        if result.peak_connections > 0:
            print(f"\n🎯 估计服务器最大并发连接数: {result.peak_connections}")
        
        print("="*70)


async def main():
    """主函数"""
    print("\n正在初始化测试器...")
    
    # 配置测试参数
    HOST = '127.0.0.1'
    PORT = 8090  # ChatServer 端口
    
    print(f"目标服务器: {HOST}:{PORT}")
    
    # 先测试服务器是否可达
    print(f"\n检查服务器连接...")
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(HOST, PORT),
            timeout=3.0
        )
        writer.close()
        await writer.wait_closed()
        print("✅ 服务器连接正常!")
    except Exception as e:
        print(f"❌ 无法连接到服务器 {HOST}:{PORT}")
        print(f"错误信息: {e}")
        print(f"\n请确保:")
        print(f"  1. ChatServer 正在运行")
        print(f"  2. MySQL 和 Redis 已启动")
        print(f"  3. 防火墙允许端口 {PORT}")
        input("\n按回车键退出...")
        sys.exit(1)
    
    tester = ConnectionStressTester(host=HOST, port=PORT)
    
    print("\n" + "="*70)
    print("请选择测试模式:")
    print("="*70)
    print("1. 递增式压力测试 (逐步增加并发数,找到上限)")
    print("2. 固定并发稳定性测试 (在指定并发数下持续测试)")
    print("="*70)
    
    try:
        choice = input("\n请输入选择 (1/2, 默认1): ").strip() or "1"
    except EOFError:
        print("\n错误: 无法读取输入,请在交互式终端中运行此脚本")
        sys.exit(1)
    
    if choice == "1":
        # 递增式测试
        try:
            start = int(input("起始并发数 (默认10): ").strip() or "10")
            max_conn = int(input("最大并发数 (默认1000): ").strip() or "1000")
            step = int(input("步进大小 (默认50): ").strip() or "50")
        except EOFError:
            print("\n使用默认参数进行测试...")
            start = 10
            max_conn = 1000
            step = 50
        
        await tester.run_incremental_test(
            start_users=start,
            max_users=max_conn,
            step=step,
            delay_between_steps=2.0
        )
        
    elif choice == "2":
        # 固定并发测试
        try:
            concurrent = int(input("并发连接数 (默认100): ").strip() or "100")
            duration = int(input("测试时长(秒) (默认30): ").strip() or "30")
        except EOFError:
            print("\n使用默认参数进行测试...")
            concurrent = 100
            duration = 30
        
        await tester.run_fixed_test(
            concurrent_users=concurrent,
            duration=duration
        )
    
    else:
        print("无效选择!")
        sys.exit(1)


if __name__ == "__main__":
    try:
        print("\n脚本开始执行...\n")
        asyncio.run(main())
        print("\n脚本执行完毕!")
    except KeyboardInterrupt:
        print("\n\n⚠️  测试被用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ 测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
        input("\n按回车键退出...")
        sys.exit(1)
