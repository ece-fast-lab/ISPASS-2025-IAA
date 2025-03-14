SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS nation (
  n_nationkey  Int32 CODEC(LZ4),
  n_name       String CODEC(LZ4),
  n_regionkey  Int32 CODEC(LZ4),
  n_comment    String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY n_nationkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576, max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO nation
FROM INFILE '/PATH_TO/data/tpc_h_data/nation.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS region (
  r_regionkey  Int32 CODEC(LZ4),
  r_name       String CODEC(LZ4),
  r_comment    String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY r_regionkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO region
FROM INFILE '/PATH_TO/data/tpc_h_data/region.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS supplier (
  s_suppkey     Int32 CODEC(LZ4),
  s_name        String CODEC(LZ4),
  s_address     String CODEC(LZ4),
  s_nationkey   Int32 CODEC(LZ4),
  s_phone       String CODEC(LZ4),
  s_acctbal     Decimal(15,2) CODEC(LZ4),
  s_comment     String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY s_suppkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO supplier
FROM INFILE '/PATH_TO/data/tpc_h_data/supplier.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS customer (
  c_custkey     Int32 CODEC(LZ4),
  c_name        String CODEC(LZ4),
  c_address     String CODEC(LZ4),
  c_nationkey   Int32 CODEC(LZ4),
  c_phone       String CODEC(LZ4),
  c_acctbal     Decimal(15,2) CODEC(LZ4),
  c_mktsegment  String CODEC(LZ4),
  c_comment     String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY c_custkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO customer
FROM INFILE '/PATH_TO/data/tpc_h_data/customer.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS part (
  p_partkey     Int32 CODEC(LZ4),
  p_name        String CODEC(LZ4),
  p_mfgr        String CODEC(LZ4),
  p_brand       String CODEC(LZ4),
  p_type        String CODEC(LZ4),
  p_size        Int32 CODEC(LZ4),
  p_container   String CODEC(LZ4),
  p_retailprice Decimal(15,2) CODEC(LZ4),
  p_comment     String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY p_partkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO part
FROM INFILE '/PATH_TO/data/tpc_h_data/part.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS partsupp (
  ps_partkey     Int32 CODEC(LZ4),
  ps_suppkey     Int32 CODEC(LZ4),
  ps_availqty    Int32 CODEC(LZ4),
  ps_supplycost  Decimal(15,2) CODEC(LZ4),
  ps_comment     String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY (ps_partkey, ps_suppkey)
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO partsupp
FROM INFILE '/PATH_TO/data/tpc_h_data/partsupp.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS orders (
  o_orderkey       Int32 CODEC(LZ4),
  o_custkey        Int32 CODEC(LZ4),
  o_orderstatus    String CODEC(LZ4),
  o_totalprice     Decimal(15,2) CODEC(LZ4),
  o_orderdate      Date CODEC(LZ4),
  o_orderpriority  String CODEC(LZ4),
  o_clerk          String CODEC(LZ4),
  o_shippriority   Int32 CODEC(LZ4),
  o_comment        String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY o_orderkey
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO orders
FROM INFILE '/PATH_TO/data/tpc_h_data/orders.tbl'
FORMAT CSV;

SET enable_deflate_qpl_codec = 1, background_pool_size = 1, max_threads = 1;
CREATE TABLE IF NOT EXISTS lineitem (
  l_orderkey      Int32 CODEC(LZ4),
  l_partkey       Int32 CODEC(LZ4),
  l_suppkey       Int32 CODEC(LZ4),
  l_linenumber    Int32 CODEC(LZ4),
  l_quantity      Decimal(15,2) CODEC(LZ4),
  l_extendedprice Decimal(15,2) CODEC(LZ4),
  l_discount      Decimal(15,2) CODEC(LZ4),
  l_tax           Decimal(15,2) CODEC(LZ4),
  l_returnflag    String CODEC(LZ4),
  l_linestatus    String CODEC(LZ4),
  l_shipdate      Date CODEC(LZ4),
  l_commitdate    Date CODEC(LZ4),
  l_receiptdate   Date CODEC(LZ4),
  l_shipinstruct  String CODEC(LZ4),
  l_shipmode      String CODEC(LZ4),
  l_comment       String CODEC(LZ4)
) ENGINE = MergeTree()
ORDER BY (l_orderkey, l_linenumber)
SETTINGS index_granularity = 8192, min_compress_block_size = 65536, max_compress_block_size = 1048576,  max_concurrent_queries = 1, max_part_loading_threads = 1;

SET format_csv_delimiter = '|';
INSERT INTO lineitem
FROM INFILE '/PATH_TO/data/tpc_h_data/lineitem.tbl'
FORMAT CSV;