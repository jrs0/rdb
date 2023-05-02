# Profiles

This folder contains profiles of the program under different conditions

## kernel_500000_rows_4_cols.svg

This is a profile of 4 diagnosis and procedure columns, and 500000 rowss. Fewer diagnosis and procedure columns, and less parsing in general, means the cache may not be full yet, so this may be profiling the parser.

## kernel_1500000_rows_10_cols.svg

More rows and columns (especially) takes much longer, and the cache should be used more, so this is a more realistic profile.


