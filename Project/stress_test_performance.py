#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LChat Server Performance & Stability Test Tool
Test message sending/receiving efficiency and stability under high concurrency

Performance Standards:
- Support 10,000 concurrent connections
- Zero packet loss rate and zero disconnection rate
- Message send/receive latency stable within 10ms

Prerequisites:
    1. Install Redis package: pip install redis
    2. Ensure Redis server is running (default: 127.0.0.1:6379)
    3. Ensure ChatServer is running and connected to MySQL and Redis

**运行方式**：

1. **交互式模式**（会提示输入所有参数，包括Redis配置）：
```bash
python stress_test_performance.py
```
终端会依次提示输入：
- Redis port (default 6379): 
- Redis password (leave empty if no password): 
- Concurrent connections (default 10000): 
- Messages per client (default 10): 
- Message interval in seconds (default 0.1): 

2. **批处理模式**（使用命令行参数，无需交互）：
```bash
python stress_test_performance.py --concurrent 10000 --messages 10 --interval 0.1 --batch
```

3. **指定服务器地址和Redis配置**：
```bash
# 带密码认证
python stress_test_performance.py --host 192.168.1.100 --port 8090 \
  --redis-host 192.168.1.100 --redis-port 6380 --redis-password YOUR_PASSWORD \
  --concurrent 10000 --messages 10 --interval 0.1 --batch

# 不带密码
python stress_test_performance.py --redis-port 6380 --concurrent 10000 --messages 10 --interval 0.1 --batch
```

Notes:
    - The script will automatically initialize test user data in Redis before testing
    - Test users will have UIDs starting from 1000 (e.g., 1000-10999 for 10000 users)
    - Each test user gets a token "test_token" stored in Redis
    - Make sure Redis server is accessible before running the test
"""

import asyncio
import time
import statistics
import sys
import struct
import json
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple
from datetime import datetime
from collections import defaultdict

try:
    import redis
    REDIS_AVAILABLE = True
except ImportError:
    REDIS_AVAILABLE = False
    print("Warning: redis package not installed. Install with: pip install redis")


print("="*70)
print("LChat Server Performance & Stability Test Tool")
print("="*70)
print(f"Python Version: {sys.version}")
print(f"Start Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
print("="*70)
print()


# Message protocol constants
HEAD_TOTAL_LEN = 4
HEAD_ID_LEN = 2
HEAD_DATA_LEN = 2

# Message IDs
MSG_CHAT_LOGIN = 1005
MSG_CHAT_LOGIN_RSP = 1006
ID_TEXT_CHAT_MSG_REQ = 1015
ID_TEXT_CHAT_MSG_RSP = 1016
ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1017


@dataclass
class MessageStats:
    """Message statistics for a single client"""
    sent_count: int = 0
    received_count: int = 0
    lost_count: int = 0
    latencies: List[float] = field(default_factory=list)
    errors: int = 0
    
    @property
    def avg_latency(self) -> float:
        if not self.latencies:
            return 0.0
        return statistics.mean(self.latencies)
    
    @property
    def p95_latency(self) -> float:
        if not self.latencies:
            return 0.0
        sorted_lat = sorted(self.latencies)
        index = int(len(sorted_lat) * 0.95)
        return sorted_lat[min(index, len(sorted_lat) - 1)]
    
    @property
    def p99_latency(self) -> float:
        if not self.latencies:
            return 0.0
        sorted_lat = sorted(self.latencies)
        index = int(len(sorted_lat) * 0.99)
        return sorted_lat[min(index, len(sorted_lat) - 1)]


@dataclass
class PerformanceTestResult:
    """Overall performance test result"""
    total_clients: int = 0
    active_clients: int = 0
    disconnected_clients: int = 0
    total_messages_sent: int = 0
    total_messages_received: int = 0
    total_messages_lost: int = 0
    all_latencies: List[float] = field(default_factory=list)
    test_duration: float = 0.0
    client_stats: Dict[int, MessageStats] = field(default_factory=dict)
    errors: Dict[str, int] = field(default_factory=dict)
    
    @property
    def packet_loss_rate(self) -> float:
        if self.total_messages_sent == 0:
            return 0.0
        return (self.total_messages_lost / self.total_messages_sent) * 100
    
    @property
    def disconnect_rate(self) -> float:
        if self.total_clients == 0:
            return 0.0
        return (self.disconnected_clients / self.total_clients) * 100
    
    @property
    def avg_latency(self) -> float:
        if not self.all_latencies:
            return 0.0
        return statistics.mean(self.all_latencies)
    
    @property
    def median_latency(self) -> float:
        if not self.all_latencies:
            return 0.0
        return statistics.median(self.all_latencies)
    
    @property
    def p95_latency(self) -> float:
        if not self.all_latencies:
            return 0.0
        sorted_lat = sorted(self.all_latencies)
        index = int(len(sorted_lat) * 0.95)
        return sorted_lat[min(index, len(sorted_lat) - 1)]
    
    @property
    def p99_latency(self) -> float:
        if not self.all_latencies:
            return 0.0
        sorted_lat = sorted(self.all_latencies)
        index = int(len(sorted_lat) * 0.99)
        return sorted_lat[min(index, len(sorted_lat) - 1)]


class ChatClient:
    """Simulated chat client for performance testing"""
    
    def __init__(self, client_id: int, host: str, port: int):
        self.client_id = client_id
        self.host = host
        self.port = port
        self.reader = None
        self.writer = None
        self.stats = MessageStats()
        self.connected = False
        self.uid = 0
        self._pending_messages = {}  # msg_id -> send_time
        self._msg_counter = 0
        
    async def connect(self) -> bool:
        """Establish connection to server"""
        try:
            self.reader, self.writer = await asyncio.wait_for(
                asyncio.open_connection(self.host, self.port),
                timeout=5.0
            )
            self.connected = True
            return True
        except Exception as e:
            self.stats.errors += 1
            return False
    
    async def login(self, uid: int, token: str = "test_token") -> bool:
        """Send login message"""
        try:
            # Build login message
            login_data = {
                "uid": uid,
                "token": token
            }
            login_json = json.dumps(login_data).encode('utf-8')
            
            # Send message with header
            success = await self._send_message(MSG_CHAT_LOGIN, login_json)
            
            if success:
                # Wait for login response
                response = await self._receive_message(timeout=3.0)
                if response:
                    msg_id, body = response
                    if msg_id == MSG_CHAT_LOGIN_RSP:
                        rsp_data = json.loads(body.decode('utf-8'))
                        error_code = rsp_data.get('error', -1)
                        if error_code == 0:
                            self.uid = uid
                            return True
                        else:
                            # Log login failure reason (only first few to avoid spam)
                            if uid <= 1005:  # Only log first 6 failures
                                print(f"      [DEBUG] Login failed for UID {uid}: error code {error_code}")
                            return False
                    else:
                        if uid <= 1005:
                            print(f"      [DEBUG] Unexpected response msg_id {msg_id} for UID {uid}")
                        return False
                else:
                    if uid <= 1005:
                        print(f"      [DEBUG] No response received for UID {uid}")
                    return False
            
            return False
        except Exception as e:
            self.stats.errors += 1
            if uid <= 1005:
                print(f"      [DEBUG] Login exception for UID {uid}: {e}")
            return False
    
    async def send_chat_message(self, to_uid: int, message: str) -> bool:
        """Send a chat message and track latency"""
        try:
            self._msg_counter += 1
            msg_id = self._msg_counter
            
            # Build chat message - match server's expected format
            chat_data = {
                "fromuid": self.uid,
                "touid": to_uid,
                "text_array": [
                    {
                        "msgid": f"msg_{msg_id}",
                        "content": message
                    }
                ],
                "timestamp": time.time()
            }
            chat_json = json.dumps(chat_data).encode('utf-8')
            
            # Record send time
            send_time = time.time()
            self._pending_messages[msg_id] = send_time
            
            # Send message
            success = await self._send_message(ID_TEXT_CHAT_MSG_REQ, chat_json)
            
            if success:
                self.stats.sent_count += 1
                return True
            else:
                del self._pending_messages[msg_id]
                return False
                
        except Exception as e:
            self.stats.errors += 1
            return False
    
    async def receive_and_process(self) -> Tuple[int, bytes]:
        """Receive and process incoming message"""
        try:
            # Use reasonable timeout - long enough for server to respond under load
            response = await self._receive_message(timeout=10.0)
            if response:
                msg_id, body = response
                
                # If it's a response message, calculate latency
                if msg_id == ID_TEXT_CHAT_MSG_RSP or msg_id == ID_NOTIFY_TEXT_CHAT_MSG_REQ:
                    try:
                        rsp_data = json.loads(body.decode('utf-8'))
                        
                        # Try to extract msgid from text_array (server format)
                        original_msg_id = None
                        if 'text_array' in rsp_data and len(rsp_data['text_array']) > 0:
                            # Server returns text_array with msgid inside
                            first_msg = rsp_data['text_array'][0]
                            msg_id_str = first_msg.get('msgid', '')
                            if msg_id_str.startswith('msg_'):
                                original_msg_id = int(msg_id_str.split('_')[1])
                        elif 'msgid' in rsp_data:
                            # Fallback: direct msgid field
                            msg_id_str = rsp_data['msgid']
                            if msg_id_str.startswith('msg_'):
                                original_msg_id = int(msg_id_str.split('_')[1])
                        
                        # Calculate latency if we found the message ID
                        if original_msg_id is not None and original_msg_id in self._pending_messages:
                            send_time = self._pending_messages.pop(original_msg_id)
                            latency = (time.time() - send_time) * 1000  # Convert to ms
                            self.stats.latencies.append(latency)
                            self.stats.received_count += 1
                    except Exception as e:
                        # Silently ignore parsing errors
                        pass
                
                return msg_id, body
            
            # Timeout occurred - message likely lost or severely delayed
            return None, None
        except asyncio.TimeoutError:
            # Expected under high load - don't count as error
            return None, None
        except Exception as e:
            self.stats.errors += 1
            self.connected = False
            return None, None
    
    async def _send_message(self, msg_id: int, data: bytes) -> bool:
        """Send message with protocol header"""
        try:
            if not self.writer:
                return False
            
            # Build header: [msg_id (2 bytes)][data_len (2 bytes)]
            data_len = len(data)
            header = struct.pack('!HH', msg_id, data_len)
            
            # Send header + data
            self.writer.write(header + data)
            await self.writer.drain()
            return True
        except Exception as e:
            self.stats.errors += 1
            self.connected = False
            return False
    
    async def _receive_message(self, timeout: float = 2.0) -> Tuple[int, bytes]:
        """Receive message with protocol header"""
        try:
            if not self.reader:
                return None, None
            
            # Read header (4 bytes)
            header_data = await asyncio.wait_for(
                self.reader.readexactly(HEAD_TOTAL_LEN),
                timeout=timeout
            )
            
            # Parse header
            msg_id, data_len = struct.unpack('!HH', header_data)
            
            # Read body
            if data_len > 0:
                body_data = await asyncio.wait_for(
                    self.reader.readexactly(data_len),
                    timeout=timeout
                )
                return msg_id, body_data
            
            return msg_id, b''
        except asyncio.TimeoutError:
            return None, None
        except Exception as e:
            self.stats.errors += 1
            self.connected = False
            return None, None
    
    async def close(self):
        """Close connection"""
        try:
            if self.writer:
                self.writer.close()
                await self.writer.wait_closed()
        except:
            pass
        finally:
            self.connected = False


class PerformanceTester:
    """Performance and stability tester for ChatServer"""
    
    def __init__(self, host: str = '127.0.0.1', port: int = 8090, 
                 redis_host: str = '127.0.0.1', redis_port: int = 6379,
                 redis_password: str = None):
        self.host = host
        self.port = port
        self.redis_host = redis_host
        self.redis_port = redis_port
        self.redis_password = redis_password
        self.result = PerformanceTestResult()
        
    def init_redis_test_data(self, user_count: int, base_uid: int = 1000) -> bool:
        """
        Initialize test user data in Redis
        
        Args:
            user_count: Number of users to initialize
            base_uid: Starting UID (default 1000)
            
        Returns:
            True if initialization successful, False otherwise
        """
        if not REDIS_AVAILABLE:
            print("❌ Redis package not available. Cannot initialize test data.")
            print("   Install with: pip install redis")
            return False
        
        try:
            print(f"\n🗄️  Initializing Redis test data...")
            print(f"   Redis Server: {self.redis_host}:{self.redis_port}")
            print(f"   User Count: {user_count}")
            print(f"   UID Range: {base_uid} - {base_uid + user_count - 1}")
            
            # Connect to Redis
            redis_params = {
                'host': self.redis_host,
                'port': self.redis_port,
                'db': 0,
                'decode_responses': True
            }
            
            # Add password if provided
            if self.redis_password:
                redis_params['password'] = self.redis_password
            
            r = redis.Redis(**redis_params)
            r.ping()  # Test connection
            
            # Batch set user tokens and base info
            pipe = r.pipeline()
            test_token = "test_token"
            
            for i in range(user_count):
                uid = base_uid + i
                uid_str = str(uid)
                
                # Set token: utoken_{uid} = test_token
                token_key = f"utoken_{uid_str}"
                pipe.set(token_key, test_token)
                
                # Set user base info: ubaseinfo_{uid} = JSON string
                base_key = f"ubaseinfo_{uid_str}"
                user_info = {
                    "uid": uid,
                    "name": f"user{uid}",
                    "email": f"user{uid}@test.com",
                    "nick": f"Nick{uid}",
                    "desc": f"Test user {uid}",
                    "sex": 1,
                    "icon": "default_icon.png",
                    "pwd": "test_pwd"
                }
                pipe.set(base_key, json.dumps(user_info))
                
                # Execute batch every 1000 users
                if (i + 1) % 1000 == 0:
                    pipe.execute()
                    print(f"   Progress: {i + 1}/{user_count} users initialized")
            
            # Execute remaining commands
            pipe.execute()
            
            print(f"✅ Redis test data initialized successfully!")
            print(f"   Total keys set: {user_count * 2}")  # token + base info per user
            return True
            
        except redis.ConnectionError as e:
            print(f"❌ Failed to connect to Redis: {e}")
            print(f"   Please ensure Redis server is running at {self.redis_host}:{self.redis_port}")
            return False
        except Exception as e:
            print(f"❌ Failed to initialize Redis data: {e}")
            import traceback
            traceback.print_exc()
            return False
        
    async def test_performance(self, 
                               concurrent_users: int = 10000,
                               messages_per_client: int = 10,
                               message_interval: float = 0.1,
                               test_name: str = "Performance Test"):
        """
        Performance test: Test message send/receive efficiency under specified concurrency
        
        Args:
            concurrent_users: Number of concurrent connections
            messages_per_client: Number of messages each client sends
            message_interval: Interval between messages (seconds)
            test_name: Test name for reporting
        """
        print("="*70)
        print(f"🚀 {test_name}")
        print("="*70)
        print(f"Target Server: {self.host}:{self.port}")
        print(f"Concurrent Users: {concurrent_users}")
        print(f"Messages per Client: {messages_per_client}")
        print(f"Message Interval: {message_interval}s")
        print("="*70)
        print()
        
        # Initialize Redis test data before starting the test
        print("📋 Pre-test Setup:")
        base_uid = 1000  # Starting UID for test users
        if not self.init_redis_test_data(concurrent_users, base_uid):
            print("⚠️  Continuing without Redis initialization (test may fail)")
        print()
        
        start_time = time.time()
        clients: List[ChatClient] = []
        self.result.total_clients = concurrent_users
        
        # Phase 1: Establish connections
        print("📡 Phase 1: Establishing connections...")
        batch_size = 100
        connected_count = 0
        
        for i in range(0, concurrent_users, batch_size):
            batch_end = min(i + batch_size, concurrent_users)
            tasks = []
            
            for j in range(i, batch_end):
                client = ChatClient(j, self.host, self.port)
                clients.append(client)
                tasks.append(client.connect())
            
            results = await asyncio.gather(*tasks, return_exceptions=True)
            
            for idx, res in enumerate(results):
                if isinstance(res, bool) and res:
                    connected_count += 1
                elif isinstance(res, Exception):
                    self.result.errors["connection_error"] = \
                        self.result.errors.get("connection_error", 0) + 1
            
            progress = (batch_end / concurrent_users) * 100
            print(f"  Progress: {batch_end}/{concurrent_users} ({progress:.1f}%) - "
                  f"Connected: {connected_count}")
            
            # Small delay to avoid overwhelming the server
            await asyncio.sleep(0.05)
        
        self.result.active_clients = connected_count
        print(f"\n✅ Connection phase completed: {connected_count}/{concurrent_users} connected")
        
        if connected_count == 0:
            print("❌ No successful connections, aborting test!")
            return self.result
        
        # Phase 2: Login
        print("\n🔐 Phase 2: Logging in users...")
        
        # Login in batches to show progress and avoid overwhelming server
        login_batch_size = 500
        total_logged_in = 0
        total_login_tasks = 0
        
        for batch_start in range(0, len(clients), login_batch_size):
            batch_end = min(batch_start + login_batch_size, len(clients))
            batch_clients = clients[batch_start:batch_end]
            
            login_tasks = []
            for client in batch_clients:
                if client.connected:
                    login_tasks.append(client.login(client.client_id + 1000))
            
            if login_tasks:
                total_login_tasks += len(login_tasks)
                login_results = await asyncio.gather(*login_tasks, return_exceptions=True)
                batch_logged_in = sum(1 for r in login_results if isinstance(r, bool) and r)
                total_logged_in += batch_logged_in
                
                progress = (batch_end / len(clients)) * 100
                print(f"  Progress: {batch_end}/{len(clients)} ({progress:.1f}%) - "
                      f"Logged in: {total_logged_in}")
                
                # Small delay between batches to give server time to process
                await asyncio.sleep(0.1)
        
        print(f"✅ Login completed: {total_logged_in}/{total_login_tasks} users logged in")
        
        # Filter only logged-in clients
        active_clients = [c for c in clients if c.connected and c.uid > 0]
        print(f"📊 Active clients for messaging: {len(active_clients)}")
        
        if len(active_clients) == 0:
            print("❌ No active clients for messaging, aborting test!")
            return self.result
        
        # Phase 3: Send and receive messages
        print(f"\n💬 Phase 3: Sending and receiving messages...")
        print(f"  Each client will send {messages_per_client} messages")
        print(f"  Total expected messages: {len(active_clients) * messages_per_client}")
        print()
        
        # Create messaging tasks
        async def client_messaging_task(client: ChatClient, target_uid: int):
            """Task for a single client to send and receive messages"""
            for msg_idx in range(messages_per_client):
                if not client.connected:
                    break
                
                # Send message
                message_content = f"Test message {msg_idx + 1} from client {client.client_id}"
                await client.send_chat_message(target_uid, message_content)
                
                # Receive response - no retry, just try once with longer timeout
                await client.receive_and_process()
                
                # Wait for interval between messages
                if msg_idx < messages_per_client - 1:
                    await asyncio.sleep(message_interval)
        
        # Pair clients for messaging (client i sends to client i+1)
        messaging_tasks = []
        for i, client in enumerate(active_clients):
            # Determine target: next client in list (or first if last)
            target_idx = (i + 1) % len(active_clients)
            target_client = active_clients[target_idx]
            
            task = client_messaging_task(client, target_client.uid)
            messaging_tasks.append(task)
        
        # Execute all messaging tasks concurrently
        print("  📤 Starting message exchange...")
        msg_start_time = time.time()
        
        # Run ALL clients at once - let the server handle the load
        # This gives us a true picture of server capacity
        print(f"  Running all {len(messaging_tasks)} clients concurrently...")
        await asyncio.gather(*messaging_tasks, return_exceptions=True)
        
        elapsed = time.time() - msg_start_time
        print(f"  ✅ All clients completed in {elapsed:.2f}s")
        
        msg_duration = time.time() - msg_start_time
        print(f"\n✅ Message exchange completed in {msg_duration:.2f}s")
        
        # Phase 4: Collect statistics
        print("\n📊 Phase 4: Collecting statistics...")
        
        total_sent = 0
        total_received = 0
        total_lost = 0
        all_latencies = []
        disconnected = 0
        
        for client in active_clients:
            stats = client.stats
            total_sent += stats.sent_count
            total_received += stats.received_count
            total_lost += (stats.sent_count - stats.received_count)
            all_latencies.extend(stats.latencies)
            
            if not client.connected:
                disconnected += 1
            
            # Store individual client stats
            self.result.client_stats[client.client_id] = stats
        
        self.result.total_messages_sent = total_sent
        self.result.total_messages_received = total_received
        self.result.total_messages_lost = max(0, total_lost)
        self.result.all_latencies = all_latencies
        self.result.disconnected_clients = disconnected
        self.result.test_duration = time.time() - start_time
        
        # Print detailed report
        self._print_performance_report(test_name)
        
        # Close all connections
        print("\n🔌 Closing connections...")
        close_tasks = [client.close() for client in clients]
        await asyncio.gather(*close_tasks, return_exceptions=True)
        print("✅ All connections closed")
        
        return self.result
    
    def _print_performance_report(self, test_name: str):
        """Print detailed performance report"""
        print("\n" + "="*70)
        print("📊 PERFORMANCE TEST REPORT")
        print("="*70)
        print(f"Test Name: {test_name}")
        print(f"Test Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Server: {self.host}:{self.port}")
        print("-"*70)
        
        # Connection stats
        print(f"\n🔗 Connection Statistics:")
        print(f"  Total Clients: {self.result.total_clients}")
        print(f"  Active Clients: {self.result.active_clients}")
        print(f"  Disconnected: {self.result.disconnected_clients}")
        print(f"  Disconnect Rate: {self.result.disconnect_rate:.2f}%")
        
        # Message stats
        print(f"\n💬 Message Statistics:")
        print(f"  Total Sent: {self.result.total_messages_sent}")
        print(f"  Total Received: {self.result.total_messages_received}")
        print(f"  Total Lost: {self.result.total_messages_lost}")
        print(f"  Packet Loss Rate: {self.result.packet_loss_rate:.2f}%")
        
        # Latency stats
        if self.result.all_latencies:
            print(f"\n⏱️  Latency Statistics:")
            print(f"  Average: {self.result.avg_latency:.2f} ms")
            print(f"  Median: {self.result.median_latency:.2f} ms")
            print(f"  P95: {self.result.p95_latency:.2f} ms")
            print(f"  P99: {self.result.p99_latency:.2f} ms")
            print(f"  Min: {min(self.result.all_latencies):.2f} ms")
            print(f"  Max: {max(self.result.all_latencies):.2f} ms")
            print(f"  Std Dev: {statistics.stdev(self.result.all_latencies):.2f} ms")
        
        # Duration
        print(f"\n⏰ Test Duration: {self.result.test_duration:.2f} seconds")
        
        # Errors
        if self.result.errors:
            print(f"\n❌ Errors:")
            for error_type, count in self.result.errors.items():
                print(f"  {error_type}: {count}")
        
        print("="*70)
        
        # Performance evaluation
        print(f"\n💡 Performance Evaluation:")
        
        # Check standards
        meets_standard = True
        
        # Check packet loss
        if self.result.packet_loss_rate == 0:
            print("  ✅ Packet Loss: ZERO (Standard: 0%)")
        else:
            print(f"  ❌ Packet Loss: {self.result.packet_loss_rate:.2f}% (Standard: 0%)")
            meets_standard = False
        
        # Check disconnect rate
        if self.result.disconnect_rate == 0:
            print("  ✅ Disconnect Rate: ZERO (Standard: 0%)")
        else:
            print(f"  ❌ Disconnect Rate: {self.result.disconnect_rate:.2f}% (Standard: 0%)")
            meets_standard = False
        
        # Check latency
        if self.result.p95_latency <= 10:
            print(f"  ✅ P95 Latency: {self.result.p95_latency:.2f} ms (Standard: ≤10ms)")
        else:
            print(f"  ❌ P95 Latency: {self.result.p95_latency:.2f} ms (Standard: ≤10ms)")
            meets_standard = False
        
        if self.result.p99_latency <= 10:
            print(f"  ✅ P99 Latency: {self.result.p99_latency:.2f} ms (Standard: ≤10ms)")
        else:
            print(f"  ⚠️  P99 Latency: {self.result.p99_latency:.2f} ms (Standard: ≤10ms)")
        
        # Overall assessment
        print("\n" + "="*70)
        if meets_standard:
            print("🎉 OVERALL RESULT: PASSED - Meets all performance standards!")
        else:
            print("⚠️  OVERALL RESULT: NEEDS IMPROVEMENT - Does not meet all standards")
        print("="*70)


async def main():
    """Main function"""
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='LChat Server Performance & Stability Test Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python stress_test_performance.py
  python stress_test_performance.py --host 192.168.1.100 --port 8090
  python stress_test_performance.py --concurrent 10000 --messages 10 --interval 0.1
        """
    )
    
    parser.add_argument('--host', type=str, default='127.0.0.1',
                       help='Server host address (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=8090,
                       help='Server port (default: 8090)')
    parser.add_argument('--redis-host', type=str, default='127.0.0.1',
                       help='Redis server host address (default: 127.0.0.1)')
    parser.add_argument('--redis-port', type=int, default=6379,
                       help='Redis server port (default: 6379)')
    parser.add_argument('--redis-password', type=str, default=None,
                       help='Redis server password (if authentication is required)')
    parser.add_argument('--concurrent', type=int, default=None,
                       help='Number of concurrent connections (default: interactive input or 10000)')
    parser.add_argument('--messages', type=int, default=None,
                       help='Messages per client (default: interactive input or 10)')
    parser.add_argument('--interval', type=float, default=None,
                       help='Message interval in seconds (default: interactive input or 0.1)')
    parser.add_argument('--batch', action='store_true',
                       help='Batch mode: use defaults without interactive prompts')
    
    args = parser.parse_args()
    
    print("\nInitializing performance tester...\n")
    
    # Configuration
    HOST = args.host
    PORT = args.port  # ChatServer port
    REDIS_HOST = args.redis_host
    REDIS_PORT = args.redis_port
    REDIS_PASSWORD = args.redis_password
    
    print(f"Target Server: {HOST}:{PORT}")
    print(f"Redis Server: {REDIS_HOST}:{REDIS_PORT}")
    if REDIS_PASSWORD:
        print(f"Redis Auth: Password authentication enabled")
    
    # Check server connectivity
    print(f"\nChecking server connectivity...")
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(HOST, PORT),
            timeout=3.0
        )
        writer.close()
        await writer.wait_closed()
        print("✅ Server is reachable!")
    except Exception as e:
        print(f"❌ Cannot connect to server {HOST}:{PORT}")
        print(f"Error: {e}")
        print(f"\nPlease ensure:")
        print(f"  1. ChatServer is running")
        print(f"  2. MySQL and Redis are started")
        print(f"  3. Firewall allows port {PORT}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    print("\n" + "="*70)
    print("PERFORMANCE TEST CONFIGURATION")
    print("="*70)
    
    # Determine test parameters
    if args.batch or (args.concurrent is not None and args.messages is not None and args.interval is not None):
        # Batch mode or all parameters provided via command line
        concurrent = args.concurrent if args.concurrent is not None else 10000
        messages = args.messages if args.messages is not None else 10
        interval = args.interval if args.interval is not None else 0.1
        
        print(f"Running in batch mode with parameters:")
        print(f"  Concurrent connections: {concurrent}")
        print(f"  Messages per client: {messages}")
        print(f"  Message interval: {interval}s")
    else:
        # Interactive mode
        try:
            # Redis configuration
            redis_port_input = input("Redis port (default 6379): ").strip()
            REDIS_PORT = int(redis_port_input) if redis_port_input else 6379
            
            redis_password_input = input("Redis password (leave empty if no password): ").strip()
            REDIS_PASSWORD = redis_password_input if redis_password_input else None
            
            # Test parameters with better defaults for high concurrency
            print("\n💡 Tip: For 10000 concurrent connections, use larger message intervals")
            print("   to avoid overwhelming the server.")
            print()
            
            concurrent = int(input("Concurrent connections (default 10000): ").strip() or "10000")
            messages = int(input("Messages per client (default 10): ").strip() or "10")
            
            # Suggest appropriate interval based on concurrent users
            default_interval = 1.0 if concurrent >= 5000 else 0.5 if concurrent >= 1000 else 0.1
            interval_input = input(f"Message interval in seconds (default {default_interval}): ").strip()
            interval = float(interval_input) if interval_input else default_interval
            
        except EOFError:
            print("\nUsing default parameters for testing...")
            REDIS_PORT = 6379
            REDIS_PASSWORD = None
            concurrent = 10000
            messages = 10
            interval = 1.0
    
    print("="*70)
    print()
    
    # Performance analysis and recommendations
    total_messages = concurrent * messages
    estimated_duration = messages * interval + 10  # Add overhead for connection/login
    
    print(f"📊 Performance Analysis:")
    print(f"   Total messages to process: {total_messages:,}")
    print(f"   Estimated test duration: ~{estimated_duration:.0f} seconds")
    
    # Check if configuration might overwhelm server
    msg_rate = total_messages / (messages * interval) if interval > 0 else float('inf')
    print(f"   Peak message rate: ~{msg_rate:.0f} messages/second")
    
    if msg_rate > 5000:
        print(f"\n⚠️  WARNING: High message rate detected!")
        print(f"   This may overwhelm a single server node.")
        print(f"   Consider:")
        print(f"   - Increasing message interval to {max(1.0, interval * 2):.1f}s")
        print(f"   - Reducing concurrent users to {min(5000, concurrent)}")
        print(f"   - Or accept higher latency/packet loss as expected under load")
        print()
    
    # Create tester after getting all configurations
    tester = PerformanceTester(host=HOST, port=PORT, 
                               redis_host=REDIS_HOST, redis_port=REDIS_PORT,
                               redis_password=REDIS_PASSWORD)
    
    # Run performance test
    await tester.test_performance(
        concurrent_users=concurrent,
        messages_per_client=messages,
        message_interval=interval,
        test_name=f"Stability Test - {concurrent} Connections"
    )


if __name__ == "__main__":
    try:
        print("\nScript starting...\n")
        asyncio.run(main())
        print("\nScript completed!")
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Error during test: {e}")
        import traceback
        traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)
