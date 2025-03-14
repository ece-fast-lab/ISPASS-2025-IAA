CREATE TABLE IF NOT EXISTS part (
  p_partkey     INT,
  p_name        VARCHAR(55),
  p_mfgr        CHAR(25),
  p_brand       CHAR(10),
  p_type        VARCHAR(25),
  p_size        INT,
  p_container   CHAR(10),
  p_retailprice DECIMAL(15,2) ,
  p_comment     VARCHAR(23)) USING columnar;
-- SELECT alter_columnar_table_set('part', compression => 'qpl', stripe_row_limit => 300000, chunk_group_row_limit => 150000);
SELECT alter_columnar_table_set('part', compression => 'qpl', stripe_row_limit => 150000, chunk_group_row_limit => 10000);
\timing
-- MODIFY the path. Example) /path_to/intel-iaa-eval/data/tpc_h_data/*.tbl
COPY part from 'part.tbl' DELIMITER '|' CSV;
\timing