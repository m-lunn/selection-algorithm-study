This serves to be running documentation of the AI goofs observed in development:

Github Copilot:
- Initial introselect implementation was identical to quickselect
- Recommended optimising BFPRT by not using std::sort to find the median of each group and using a custom introsort implementation instead because it would reduce overhead. Upon further research, std::sort already uses introsort for small arrays.


Claude:
- The generated benchmarking tool made its own implementations of the algorithms and did not try to use the ones we wrote.
- Suggested testing BFPRT against another algorithm’s “killer” array and recommended this to highlight where BFPRT would excel.