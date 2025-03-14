CREATE TABLE IF NOT EXISTS supplier (
  s_suppkey     INT,
  s_name        CHAR(25),
  s_address     VARCHAR(40),
  s_nationkey   INT,
  s_phone       CHAR(15),
  s_acctbal     DECIMAL(15,2),
  s_comment     VARCHAR(101)) USING columnar;
-- SELECT alter_columnar_table_set('supplier', compression => 'lz4', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('supplier', compression => 'lz4', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY supplier from 'supplier.tbl' DELIMITER '|' CSV;
\timing