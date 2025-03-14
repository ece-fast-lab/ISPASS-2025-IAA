CREATE TABLE IF NOT EXISTS orders (
  o_orderkey       INT,
  o_custkey        INT,
  o_orderstatus    CHAR(1),
  o_totalprice     DECIMAL(15,2),
  o_orderdate      DATE,
  o_orderpriority  CHAR(15),
  o_clerk          CHAR(15),
  o_shippriority   INT,
  o_comment        VARCHAR(79)) USING columnar;
-- SELECT alter_columnar_table_set('orders', compression => 'qpl', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('orders', compression => 'qpl', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY orders from 'orders.tbl' DELIMITER '|' CSV;
\timing