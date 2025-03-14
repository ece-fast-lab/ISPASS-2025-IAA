CREATE TABLE IF NOT EXISTS partsupp (
  ps_partkey     INT,
  ps_suppkey     INT,
  ps_availqty    INT,
  ps_supplycost  DECIMAL(15,2),
  ps_comment     VARCHAR(199)) USING columnar;
-- SELECT alter_columnar_table_set('partsupp', compression => 'pglz', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('partsupp', compression => 'pglz', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY partsupp from 'partsupp.tbl' DELIMITER '|' CSV;
\timing