CREATE TABLE IF NOT EXISTS region (
  r_regionkey  INT,
  r_name       CHAR(25),
  r_comment    VARCHAR(152)) USING columnar;
-- SELECT alter_columnar_table_set('region', compression => 'lz4', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('region', compression => 'lz4', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY region from 'region.tbl' DELIMITER '|' CSV;
\timing