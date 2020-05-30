cc_library(
    name = 'networking',
    srcs = [
            'Acceptor.cpp',
            'EventLoop.cpp', 
            'Poller.cpp',
            'Channel.cpp',
            'EventLoopThreadPool.cpp',
            'TcpConnection.cpp',
            'Buffer.cpp',
            'CurrentThread.cpp',
            'Socket.cpp',
            'Thread.cpp',
            'WorkQueue.cpp'
        ],
    hdrs = [
        'Buffer.h',
        'CurrentThread.h',
        'EventLoop.h',
        'EventLoopThreadPool.h',
        'TcpConnection.h',
        'Channel.h',
        'Poller.h',
        'Acceptor.h',
        'Socket.h',
        'Cache.h',
        'EventLoopThread.h',
        'WorkQueue.h',
        'Thread.h'
    ],
    copts = [
        '-std=c++11',
    ],
    includes = ['.'],
)

cc_binary(
    name = 'simplekvsvr_benchmark',
    srcs = [
        'SimpleKVBenchmark.cpp', 
        'SimpleKVBenchmark.h', 
    ],
    linkopts = [
        '-lpthread', '-lrt',
    ],
    copts = [
        '-std=c++11', '-g', '-Og',
    ],
    deps = [':networking'],
)

cc_binary(
    name = 'simplekvsvr',
    srcs = [
        'SimpleKVSvr.cpp',
        'SimpleKVSvr.h',
        'File.h',
        'File.cpp',
        'Protocol.h',
        'Protocol.cpp',
        'LRUCache.h',
        'LRUCache.cpp',
        'Cache.h', 
        'Encoding.h',
        'SimpleKVSvrLogic.cpp', 
        'Utils.cpp'
    ],
    linkopts = [
        '-lpthread',
    ],
    copts = [
        '-std=c++11', '-g', '-Og',
    ],
    deps = [':networking'],
)