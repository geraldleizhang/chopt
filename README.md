# chopt
Source code for CHOPT paper on SIGMETRICS'20

To install CHOPT, run bash install.sh

executation files:
        main.cpp        parse the original file, calculate CHOPT result
        analysis.cpp    analyze the generated result, monitor cache policies
        analysis.py     draw figures for comparing latency of different cache policies
        pattern.py      draw figures for showing cache result of two different policies
        tinylfu.py      draw figures for comparing features of tinylfu

result folders:
        result          CHOPT result
        log             CHOPT log
        cache_behavior  cache result for different policies
        pattern_result  cache result figures by comparing two different policies
        runtime         CHOPT runtime log
        simulation      latency of different cache policies
        tinylfu         cache result figures of different configured tinylfu

code folders:
        src             all cache policies, monitors, utils
