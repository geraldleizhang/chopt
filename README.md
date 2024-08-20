# CHOPT
Source code for CHOPT paper on SIGMETRICS'20: https://geraldleizhang.com/publications/CHOPT_Sigmetrics20.pdf

## Install
Run `bash install.sh`

## Codes
- main.cpp        parse the original file, calculate CHOPT result
- analysis.cpp    analyze the generated result, monitor cache policies
- analysis.py     draw figures for comparing latency of different cache policies
- pattern.py      draw figures for showing cache result of two different policies
- tinylfu.py      draw figures for comparing features of tinylfu
- src/            all cache policies, monitors, utils
