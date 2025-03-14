CREATE TABLE IF NOT EXISTS customer (
  c_custkey     INT,
  c_name        VARCHAR(25),
  c_address     VARCHAR(40),
  c_nationkey   INT,
  c_phone       CHAR(15),
  c_acctbal     DECIMAL(15,2),
  c_mktsegment  CHAR(10),
  c_comment     VARCHAR(117)) USING columnar;
-- SELECT alter_columnar_table_set('customer', compression => 'zstd', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('customer', compression => 'zstd', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY customer from 'customer.tbl' DELIMITER '|' CSV;
\timing
