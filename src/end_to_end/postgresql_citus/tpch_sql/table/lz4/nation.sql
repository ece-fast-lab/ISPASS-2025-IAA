CREATE TABLE IF NOT EXISTS nation (
  n_nationkey  INT,
  n_name       CHAR(25),
  n_regionkey  INT,
  n_comment    VARCHAR(152)) USING columnar;
-- SELECT alter_columnar_table_set('nation', compression => 'lz4', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('nation', compression => 'lz4', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY nation from 'nation.tbl' DELIMITER '|' CSV;
\timing