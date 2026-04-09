#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_workGuards(size), _nextIOService(0) {
    for (std::size_t i = 0; i < size; ++i) {
        _workGuards[i] = std::make_unique<WorkGuard>(
            boost::asio::make_work_guard(_ioServices[i])
        );
    }

    //遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
            });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}

void AsioIOServicePool::Stop() {
    for (auto& guard : _workGuards) {
        if (guard) {
            guard.reset();
        }
    }

    // 等待所有线程结束
    for (auto& thread : _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    _threads.clear();
}